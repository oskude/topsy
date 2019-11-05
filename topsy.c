#include <cairo-xcb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

struct Color {
	double r;
	double g;
	double b;
	double a;
};

int Window_Width = 64;
int Bar_Height   = 10;
int Bar_Gap      = 1;

struct Color Color_Back       = {0.0, 0.0, 0.0, 1.0};
struct Color Color_Cpu_Used   = {0.0, 1.0, 0.0, 0.2};
struct Color Color_Cpu_Fade   = {0.0, 0.0, 0.0, 0.2};
struct Color Color_Mem_Back   = {0.0, 0.0, 0.0, 0.0};
struct Color Color_Mem_Used   = {0.09, 0.373, 0.651, 1.0};
struct Color Color_Mem_Cached = {0.07, 0.286, 0.502, 1.0};

int Window_Height;
int Next_Bar_Ypos;
int Mem_Cached_Width;
int Mem_Used_Width;

xcb_connection_t *Xcb_Conn;
cairo_t         *Cairo_Ctx;
FILE             *Top_File;

void create_window () {
	                Xcb_Conn = xcb_connect(NULL, NULL);
	const xcb_setup_t *setup = xcb_get_setup(Xcb_Conn);
	xcb_screen_t     *screen = xcb_setup_roots_iterator(setup).data;
	xcb_window_t      window = xcb_generate_id(Xcb_Conn);
	const int    mask_vals[] = { XCB_EVENT_MASK_EXPOSURE };

	// calculate our height
	Window_Height += (Bar_Height + Bar_Gap) * get_nprocs_conf();
	Window_Height += Bar_Height;

	// create window with default settings
	xcb_create_window(
		Xcb_Conn,
		XCB_COPY_FROM_PARENT,
		window,
		screen->root,
		0, 0, // x,y
		Window_Width, Window_Height,
		0, // border width
		XCB_WINDOW_CLASS_INPUT_OUTPUT, // TODO: what does this do?
		screen->root_visual,
		XCB_CW_EVENT_MASK,
		mask_vals
	);

	// change window name
	xcb_icccm_set_wm_name(Xcb_Conn, window, XCB_ATOM_STRING, 8,
		strlen("topsy"),
		"topsy"
	);

	// change window type to dock
	xcb_ewmh_connection_t     ewmh_conn;
	xcb_intern_atom_cookie_t *ewmh_cookie;
	ewmh_cookie = xcb_ewmh_init_atoms(Xcb_Conn, &ewmh_conn);
	xcb_ewmh_init_atoms_replies(&ewmh_conn, ewmh_cookie, NULL);
	xcb_ewmh_set_wm_window_type(&(ewmh_conn), window,
		1,
		&(ewmh_conn._NET_WM_WINDOW_TYPE_DOCK)
	);

	// openbox seems to need more info than above, to put us in its dock area
	// https://gitlab.com/o9000/tint2/blob/master/src/panel.c#L937
	// https://gitlab.com/o9000/tint2/issues/465
	// ps. white borders (its actually 2 different borders with Clearlook theme) comes from openbox theme, `osd.border.width`, and `osd.bg` having "border"
	xcb_icccm_wm_hints_t wmhints;
	memset(&wmhints, 0, sizeof(wmhints));
	wmhints.flags = XCB_ICCCM_WM_HINT_STATE;
	wmhints.initial_state = XCB_ICCCM_WM_STATE_WITHDRAWN;
	xcb_icccm_set_wm_hints(Xcb_Conn, window, &wmhints);

	// show window
	xcb_map_window(Xcb_Conn, window);

	// create cairo context
	int i = 0; // TODO: do we really need this for loop?
	xcb_depth_iterator_t depth = xcb_screen_allowed_depths_iterator(screen);
	for (i = 0; depth.data != NULL; ++i, xcb_depth_next(&depth))
		if (depth.data->depth == screen->root_depth)
			break;
	cairo_surface_t *surface = cairo_xcb_surface_create(
		Xcb_Conn,
		window,
		xcb_depth_visuals(depth.data),
		Window_Width,
		Window_Height
	);
	Cairo_Ctx = cairo_create(surface);

	xcb_flush(Xcb_Conn);
}

void set_cairo_color (struct Color *color) {
	cairo_set_source_rgba(Cairo_Ctx,
		color->r,
		color->g,
		color->b,
		color->a
	);
}

void redraw () {
	Mem_Used_Width = 0;
	// fill background
	set_cairo_color(&Color_Back);
	cairo_rectangle(Cairo_Ctx, 0, 0, Window_Width, Window_Height);
	cairo_fill(Cairo_Ctx);
}

void draw_topbars () {
	static char  line[128];
	static char *field;
	static float cpu_usage;
	static int   cpu_nr;
	static int   jiff_total_list[64];
	static int   jiff_total;
	static int   jiff_used_list[64];
	static int   jiff_used;
	static int   mem_cached_width;
	static int   mem_cached;
	static int   mem_total;
	static int   mem_used_width;
	static int   mem_used;

	cpu_nr = 0;
	rewind(Top_File);

	while (fgets(line, 128, Top_File)) {
		field = strtok(line, " ");

		if (strcmp("cpu", field) == 0) {
			jiff_total = atoi(strtok(NULL, " "));
			jiff_used  = atoi(strtok(NULL, " "));
			cpu_usage  = (float) (jiff_used  - jiff_used_list[cpu_nr]);
			cpu_usage /= (float) (jiff_total - jiff_total_list[cpu_nr]);

			// draw fader
			set_cairo_color(&Color_Cpu_Fade);
			cairo_rectangle(Cairo_Ctx, 0, Next_Bar_Ypos, Window_Width, Bar_Height);
			cairo_fill(Cairo_Ctx);
			// draw bar
			set_cairo_color(&Color_Cpu_Used);
			cairo_rectangle(Cairo_Ctx, 0, Next_Bar_Ypos, Window_Width * cpu_usage, Bar_Height);
			cairo_fill(Cairo_Ctx);

			jiff_total_list[cpu_nr] = jiff_total;
			jiff_used_list[cpu_nr]  = jiff_used;
			Next_Bar_Ypos += Bar_Height + Bar_Gap;
			cpu_nr++;
		}
		else
		if (strcmp("mem", field) == 0) {
			mem_total  = atoi(strtok(NULL, " "));
			mem_used   = atoi(strtok(NULL, " "));
			mem_cached = atoi(strtok(NULL, " "));
			mem_used_width   = Window_Width * ( (float)mem_used / (float)mem_total );
			mem_cached_width = mem_used_width * ( (float)mem_cached / (float)mem_used );

			if (
				Mem_Used_Width == mem_used_width
				&& Mem_Cached_Width == mem_cached_width
			) {
				continue;
			}

			Mem_Used_Width = mem_used_width;
			Mem_Cached_Width = mem_cached_width;

			// draw background
			set_cairo_color(&Color_Mem_Back);
			cairo_rectangle(Cairo_Ctx, 0, Next_Bar_Ypos, Window_Width, Bar_Height);
			cairo_fill(Cairo_Ctx);
			// draw used
			set_cairo_color(&Color_Mem_Used);
			cairo_rectangle(Cairo_Ctx, 0, Next_Bar_Ypos, mem_used_width, Bar_Height);
			cairo_fill(Cairo_Ctx);
			// draw cached
			set_cairo_color(&Color_Mem_Cached);
			cairo_rectangle(Cairo_Ctx,
				mem_used_width - mem_cached_width,
				Next_Bar_Ypos, 
				mem_cached_width,
				Bar_Height
			);
			cairo_fill(Cairo_Ctx);

			Next_Bar_Ypos += Bar_Height + Bar_Gap;
		}
	}

	xcb_flush(Xcb_Conn); // without this, nothing is drawn.
}

void handle_events ()
{
	static xcb_generic_event_t *event;

	event = xcb_poll_for_event(Xcb_Conn);

	if (event == NULL) {
		return;
	}

	if (event->response_type == XCB_EXPOSE) {
		redraw();
	}
}

struct Color parse_hex_color (char *hex) {
	int dec_r, dec_g, dec_b, dec_a = 255;

	sscanf(hex, "%2x%2x%2x%2x", &dec_r, &dec_g, &dec_b, &dec_a);
	struct Color color = {
		dec_r / 255.0,
		dec_g / 255.0,
		dec_b / 255.0,
		dec_a / 255.0
	};

	return color;
}

void load_config (char *path) {
	FILE *file;
	char name[32];
	char val[32];

	file = fopen(path, "r");
	if (file == NULL) {
		return;
	}
	while (fscanf(file, "%s %s\n", name, val) != EOF) {
		if (strcmp(name, "color_back") == 0)
			Color_Back = parse_hex_color(val);
		else if (strcmp(name, "color_cpu_used") == 0)
			Color_Cpu_Used = parse_hex_color(val);
		else if (strcmp(name, "color_cpu_fade") == 0)
			Color_Cpu_Fade = parse_hex_color(val);
		else if (strcmp(name, "color_mem_back") == 0)
			Color_Mem_Back = parse_hex_color(val);
		else if (strcmp(name, "color_mem_used") == 0)
			Color_Mem_Used = parse_hex_color(val);
		else if (strcmp(name, "color_mem_cached") == 0)
			Color_Mem_Cached = parse_hex_color(val);
	}

	fclose(file);
}

void cleanup () {
	printf("::cleanup\n");

	fclose(Top_File);
	xcb_disconnect(Xcb_Conn);

	exit(1);
}

int main (int argc, char **argv) {
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);

	// TODO: read from XDG_CONFIG_DIRS XDG_CONFIG_HOME
	if (access("/etc/xdg/topsy.conf", R_OK) != -1)
		load_config("/etc/xdg/topsy.conf");

	Top_File = fopen("/proc/topstat", "r");
	create_window();

	while (1) {
		Next_Bar_Ypos = 0;
		handle_events();
		draw_topbars();
		usleep(1000 * 250);
	}

	return 0;
}
