#include <obs-module.h>
#include <util/platform.h>

struct custom_effects
{
	obs_source_t *source;
	gs_effect_t *effect;
};

static const char *custom_effects_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Name");
}

static void custom_effects_destroy(void *data)
{
	struct custom_effects *filter = data;
	if (!filter) return;

	obs_enter_graphics();
	gs_effect_destroy(filter->effect);
	obs_leave_graphics();
	bfree(filter);
}

static void custom_effects_update(void *data, obs_data_t *settings)
{
	const char *effect = obs_data_get_string(settings, "effect");
	if (!effect) return;

	struct custom_effects *filter = data;

	obs_enter_graphics();

	gs_effect_destroy(filter->effect);

	char *file = obs_module_file(effect);
	filter->effect = gs_effect_create_from_file(file, 0);
	bfree(file);

	if (!filter->effect)
	{
		custom_effects_destroy(filter);
		filter = 0;
	}

	obs_leave_graphics();
}

static void *custom_effects_create(obs_data_t *settings, obs_source_t *source)
{
	struct custom_effects *filter = bzalloc(sizeof(struct custom_effects));
	filter->source = source;
	custom_effects_update(filter, settings);
	return filter;
}

static obs_properties_t *custom_effects_properties(void *data)
{
	obs_properties_t *ppts = obs_properties_create();

	obs_property_t *list = obs_properties_add_list(ppts, "effect", obs_module_text("Label"), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	os_dir_t *dir = os_opendir(obs_get_module_data_path(obs_current_module()));

	for (;;)
	{
		struct os_dirent *entry = os_readdir(dir);
		if (!entry) break;

		if (entry->directory) continue;

		char *ext = strstr(entry->d_name, ".effect");
		if (!ext) continue;

		char *friendly = bstrdup_n(entry->d_name, sizeof(char) * (ext - entry->d_name));
		obs_property_list_add_string(list, friendly, entry->d_name);
		bfree(friendly);
	}

	os_closedir(dir);

	UNUSED_PARAMETER(data);
	return ppts;
}

static void custom_effects_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "effect", "default.effect");
}

static void custom_effects_render(void *data, gs_effect_t *effect)
{
	struct custom_effects *filter = data;

	if (!obs_source_process_filter_begin(filter->source, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) return;

	obs_source_process_filter_end(filter->source, filter->effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

struct obs_source_info gameboy_source =
{
	.id = "custom-effects",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = custom_effects_name,
	.create = custom_effects_create,
	.destroy = custom_effects_destroy,
	.get_properties = custom_effects_properties,
	.get_defaults = custom_effects_defaults,
	.update = custom_effects_update,
	.video_render = custom_effects_render
};

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE("custom-effects", "en-US")

bool obs_module_load(void)
{
	obs_register_source(&gameboy_source);
	return true;
}