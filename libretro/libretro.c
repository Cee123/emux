#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <audio.h>
#include <cmdline.h>
#include <libretro.h>
#include <log.h>
#include <machine.h>
#include <video.h>

/* Package definitions */
#define PACKAGE_NAME	"emux"
#define PACKAGE_VERSION	"0.1"

/* Retro frontends functions */
void retro_audio_fill_timing(struct retro_system_timing *timing);
void retro_video_fill_timing(struct retro_system_timing *timing);
void retro_video_fill_geometry(struct retro_game_geometry *geometry);
bool retro_video_updated();

retro_environment_t retro_environment_cb;
static bool machine_initialized;

void retro_init(void)
{
	char *system_dir;
	char *config_dir;
	char *save_dir;
	struct retro_log_callback log_callback;

	/* Get system directory from frontend */
	retro_environment_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY,
		&system_dir);

	/* Get config directory from frontend (currently equal to system dir) */
	retro_environment_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY,
		&config_dir);

	/* Get save directory from frontend */
	retro_environment_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir);

	/* Override log callback if supported by frontend */
	if (retro_environment_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE,
		&log_callback))
		log_cb = (log_print_t)log_callback.log;

	/* Set system/config/save directories */
	cmdline_set_param("system-dir", NULL, system_dir);
	cmdline_set_param("config-dir", NULL, config_dir);
	cmdline_set_param("save-dir", NULL, save_dir);

	/* Set machine to be run */
	cmdline_set_param("machine", NULL, MACHINE);

	/* Set retro frontends */
	cmdline_set_param("audio", NULL, "retro");
	cmdline_set_param("video", NULL, "retro");
}

void retro_deinit(void)
{
}

unsigned int retro_api_version(void)
{
	return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned int port, unsigned int device)
{
	(void)port;
	(void)device;
}

void retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
	info->library_name = PACKAGE_NAME " (" MACHINE ")";
	info->library_version = PACKAGE_VERSION;
	info->need_fullpath = true;
	info->valid_extensions = VALID_EXTS;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	/* Let frontends fill timing and geometry */
	retro_audio_fill_timing(&info->timing);
	retro_video_fill_timing(&info->timing);
	retro_video_fill_geometry(&info->geometry);
}

void retro_set_environment(retro_environment_t cb)
{
	/* Set retro environment callback */
	retro_environment_cb = cb;
}

void retro_reset(void)
{
	/* Reset machine */
	machine_reset();
}

void retro_run(void)
{
	/* Run until screen is updated */
	while (!video_updated())
		machine_step();
}

bool retro_load_game(const struct retro_game_info *info)
{
	/* Set data path */
	cmdline_set_param(NULL, NULL, (char *)info->path);

	/* Initialize machine */
	if (!machine_init()) {
		LOG_E("Failed to initialize machine!\n");
		return false;
	}
	
	/* Flag machine as initialized */
	machine_initialized = true;

	/* Start audio processing */
	audio_start();

	return true;
}

void retro_unload_game(void)
{
	/* Return already if machine was not initialized */
	if (!machine_initialized)
		return;

	/* Stop audio processing */
	audio_stop();

	/* Deinitialize machine */
	machine_deinit();
	machine_initialized = false;
}

unsigned int retro_get_region(void)
{
	return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned int type,
	const struct retro_game_info *info, size_t num)
{
	(void)type;
	(void)info;
	(void)num;
	return false;
}

size_t retro_serialize_size(void)
{
	return 0;
}

bool retro_serialize(void *data_, size_t size)
{
	return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
	return false;
}

void *retro_get_memory_data(unsigned int id)
{
	if ( id == RETRO_MEMORY_SYSTEM_RAM )
		return g_ram_data;
	return NULL;
}

size_t retro_get_memory_size(unsigned int id)
{
	if ( id == RETRO_MEMORY_SYSTEM_RAM )
		return g_ram_size;
	return 0;
}

void retro_cheat_reset(void)
{
}

void retro_cheat_set(unsigned int index, bool enabled, const char *code)
{
	(void)index;
	(void)enabled;
	(void)code;
}

