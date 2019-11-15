#include <obs-module.h>
#include <util/platform.h>

#define DEFAULT_SOURCE "uniform float4x4 ViewProj;uniform texture2d image;sampler_state A{AddressU=Clamp;AddressV=Clamp;Filter=Linear;};struct B{float4 p:POSITION;float2 t:TEXCOORD0;};B V(B v){B o;o.p=mul(float4(v.p.xyz, 1.0),ViewProj);o.t=v.t;return o;}int L(int l, int o, int i){return (step(abs(i-o),0)*(l/16384%2))+(step(abs(i-o+1),0)*(l/8192%2))+(step(abs(i-o+2),0)*(l/4096%2))+(step(abs(i-o+32),0)*(l/2048%2))+(step(abs(i-o+33),0)*(l/1024%2))+(step(abs(i-o+34),0)*(l/512%2))+(step(abs(i-o+64),0)*(l/256%2))+(step(abs(i-o+65),0)*(l/128%2))+(step(abs(i-o+66),0)*(l/64%2))+(step(abs(i-o+96),0)*(l/32%2))+(step(abs(i-o+97),0)*(l/16%2))+(step(abs(i-o+98),0)*(l/8 %2))+(step(abs(i-o+128),0)*(l/4%2))+(step(abs(i-o+129),0)*(l/2%2))+(step(abs(i-o+130),0)*(l%2));}float4 P(B f):TARGET{int2 r=floor(f.t*32);int p=r.y*32+r.x;int e=L(29391,552,p)+L(23275,556,p)+L(23275,560,p)+L(11114,564,p)+L(23275,568,p);float4 c=image.Sample(A,r*0.03125);float i=(c.x+c.y+c.z)*0.1;return (1-e)*float4(i,i,i,1)+e*float4(1,0,0,1);}technique Draw{pass{vertex_shader=V(v);pixel_shader=P(f);}}"

struct custom_effects
{
	obs_source_t *source;
	gs_effect_t *current_effect;
	const char *selected_file;
};

static bool custom_effects_reload(obs_properties_t *props, obs_property_t *property, void *data)
{
	struct custom_effects *filter = data;

	obs_enter_graphics();

	gs_effect_destroy(filter->current_effect);

	char *effect_source = os_quick_read_utf8_file(filter->selected_file);
	filter->current_effect = gs_effect_create(effect_source, 0, 0);
	bfree(effect_source);

	if (!filter->current_effect) filter->current_effect = gs_effect_create(DEFAULT_SOURCE, 0, 0);

	obs_leave_graphics();
	return true;
}

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
	gs_effect_destroy(filter->current_effect);
	obs_leave_graphics();
	bfree(filter);
}

static void custom_effects_update(void *data, obs_data_t *settings)
{
	struct custom_effects *filter = data;
	filter->selected_file = obs_data_get_string(settings, "path_effect");
	custom_effects_reload(0, 0, data);
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
	struct custom_effects *filter = data;

	obs_properties_t *ppts = obs_properties_create();

	obs_properties_add_path(ppts, "path_effect", obs_module_text("Path.Effect"), OBS_PATH_FILE, "*.effect", 0);
	obs_properties_add_button(ppts, "btn_reload", obs_module_text("Button.Reload"), custom_effects_reload);

	return ppts;
}

static void custom_effects_defaults(obs_data_t *settings)
{
}

static void custom_effects_render(void *data, gs_effect_t *effect)
{
	struct custom_effects *filter = data;

	if (!obs_source_process_filter_begin(filter->source, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) return;

	obs_source_process_filter_end(filter->source, filter->current_effect, 0, 0);

	UNUSED_PARAMETER(effect);
}

struct obs_source_info custom_effects_source =
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
	obs_register_source(&custom_effects_source);
	return true;
}