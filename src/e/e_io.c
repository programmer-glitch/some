#include "rhc/log.h"
#include "rhc/str.h"
#include "rhc/string.h"
#include "rhc/file.h"
#include "e/io.h"


static bool savestate_filename_valid(const char *filename) {
    Str_s name = strc(filename);
    bool valid = str_count(name, '/') == 0 && name.size < E_IO_SAVESTATE_MAX_FILENAME_LENGTH;
    if(!valid)
        log_error("e_io_savestate failed: filename not valid: %s", filename);
    return valid;
}


#ifdef __EMSCRIPTEN__

#include "emscripten.h"

static struct {
    bool mounted;
    bool synced;
} L;

static void idbfs_mount() {
    if(L.mounted)
        return;
    EM_ASM( 
        FS.mkdir('/savestate'); 
        FS.mount(IDBFS, {}, '/savestate'); 
    );
    L.mounted = true;
    log_info("e_io_idbfs_mount");
}

// protectec, JS may need its name as global
void e_io_idbfs_synced() {
    log_trace("e_io_idbfs_synced");
    L.synced = true;
}

static void idbfs_load() {
    L.synced = false;
    
    // true = load file system from idbfs
    EM_ASM(
        FS.syncfs(true, function (err) { 
            assert(!err); 
            ccall('e_io_idbfs_synced', 'v'); 
        });
    );
    // sleep a ms until synced to get a synchronous call
    while(!L.synced) {
        emscripten_sleep(1);
    }
}


static void idbga_save() {
    L.synced = false;
    
    // false = load file system from idbfs
    EM_ASM(
        FS.syncfs(false, function (err) { 
            assert(!err); 
            ccall('e_io_idbfs_synced', 'v'); 
        });
    );
    // sleep a ms until synced to get a synchronous call
    while(!L.synced) {
        emscripten_sleep(1);
    }  
}

String e_io_savestate_read(const char *filename, bool ascii) {
    if(!savestate_filename_valid(filename))
        return string_new_invalid();
    char name[16 + E_IO_SAVESTATE_MAX_FILENAME_LENGTH];
    idbfs_mount();
    idbfs_load();
    snprintf(name, sizeof name, "savestate/%s", filename);
}


bool e_io_savestate_write(const char *filename, Str_s content, bool ascii) {
    if(!savestate_filename_valid(filename))
        return false;
    char name[16 + E_IO_SAVESTATE_MAX_FILENAME_LENGTH];
    idbfs_mount();
    snprintf(name, sizeof name, "savestate/%s", filename);
    idbfs_save();
}


bool e_io_savestate_append(const char *filename, Str_s content, bool ascii) {
    if(!savestate_filename_valid(filename))
        return false;
    char name[16 + E_IO_SAVESTATE_MAX_FILENAME_LENGTH];
    idbfs_mount();
    idbfs_load();
    snprintf(name, sizeof name, "savestate/%s", filename);
    idbfs_save();
}

#else

String e_io_savestate_read(const char *filename, bool ascii) {
    if(!savestate_filename_valid(filename))
        return string_new_invalid();
    char name[16 + E_IO_SAVESTATE_MAX_FILENAME_LENGTH];
    snprintf(name, sizeof name, "savestate_%s", filename);
    return file_read(name, ascii);
}


bool e_io_savestate_write(const char *filename, Str_s content, bool ascii) {
    if(!savestate_filename_valid(filename))
        return false;
    char name[16 + E_IO_SAVESTATE_MAX_FILENAME_LENGTH];
    snprintf(name, sizeof name, "savestate_%s", filename);
    return file_write(name, content, ascii);
}


bool e_io_savestate_append(const char *filename, Str_s content, bool ascii) {
    if(!savestate_filename_valid(filename))
        return false;
    char name[16 + E_IO_SAVESTATE_MAX_FILENAME_LENGTH];
    snprintf(name, sizeof name, "savestate_%s", filename);
    return file_append(name, content, ascii);
}


#endif
