#include "fmt/base.h"
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>
#include <xcb/xproto.h>
extern "C" {
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_ewmh.h>
}
#include <iostream>
#include <toml++/toml.hpp>

int SCREEN_N = 0;
constexpr uint32_t ALL_DESKTOPS = 0xFFFFFFFF;


struct Monitor {
    std::string name;
    int16_t x, y;
    uint16_t w, h;
    bool primary;
};

std::vector<Monitor> load_monitors(xcb_connection_t* c, xcb_screen_t* s) {
    auto x = xcb_randr_get_monitors(c, s->root,0);
    auto val = xcb_randr_get_monitors_reply(c, x, nullptr);
    std::vector<Monitor> arr;
    if (val) {
        auto m_iter = xcb_randr_get_monitors_monitors_iterator(val);
        while (m_iter.rem) {
            Monitor mon;
            mon.name = *&m_iter.data->name;
            mon.x = *&m_iter.data->x;
            mon.y = *&m_iter.data->y;
            mon.w = *&m_iter.data->width;
            mon.h = *&m_iter.data->height;
            arr.push_back(mon);
            xcb_randr_monitor_info_next(&m_iter);
        }
        free(val);
    }
    return arr;
}


const Monitor* pick_monitor(const std::vector<Monitor>& mons,
    int frame_x, int frame_y, int frame_w, int frame_h) {
        // TODO: find mon
}


int get_number_of_desktops(xcb_ewmh_connection_t *ewmh){
    auto num_of_ds = xcb_ewmh_get_number_of_desktops(ewmh, SCREEN_N);
    uint32_t num_of_desktops;
    if (xcb_ewmh_get_number_of_desktops_reply(ewmh, num_of_ds, &num_of_desktops, nullptr)){
        return num_of_desktops;
    }
    return -1;
}

int get_current_desktop(xcb_ewmh_connection_t *ewmh){
    auto ck = xcb_ewmh_get_current_desktop(ewmh, SCREEN_N);
    uint32_t cur = 0;
    if (!xcb_ewmh_get_current_desktop_reply(ewmh, ck, &cur, nullptr)) return -1;
    return static_cast<int>(cur);
}

// std::string p_from_pid(int pid){

// }

#define GEOM_TO_TOML(g) toml::table{ \
    {"x", g->x}, {"y", g->y}, {"w", g->width}, {"h", g->height} \
}

#define NGEOM_TO_TOML(g) toml::table{ \
    {"x", g.x}, {"y", g.y}, {"w", g.width}, {"h", g.height} \
}


// xcb_atom_t name;
// uint8_t    primary;
// uint8_t    automatic;
// uint16_t   nOutput;
// int16_t    x;
// int16_t    y;
// uint16_t   width;
// uint16_t   height;
// uint32_t   width_in_millimeters;
// uint32_t   height_in_millimeters;

bool get_frame_extents(xcb_ewmh_connection_t* ewmh, xcb_window_t win,
    xcb_ewmh_get_extents_reply_t& out) {
    auto ck = xcb_ewmh_get_frame_extents(ewmh, win);
    return xcb_ewmh_get_frame_extents_reply(ewmh, ck, &out, nullptr);
}


bool capture_desktop(xcb_connection_t *c, xcb_ewmh_connection_t *ewmh, xcb_screen_t* s){
    int curr_desktop = get_current_desktop(ewmh);
    toml::array windows;

    auto monitors = load_monitors(c, s);

    auto nc_cookie = xcb_ewmh_get_client_list(ewmh, SCREEN_N);
    xcb_ewmh_get_windows_reply_t client_list;
    if (xcb_ewmh_get_client_list_reply(ewmh, nc_cookie, &client_list, nullptr)){
        for (unsigned int i = 0; i < client_list.windows_len; ++i){
            auto ck = xcb_ewmh_get_wm_desktop(ewmh, client_list.windows[i]);
            uint32_t d = 0;
            if (!xcb_ewmh_get_wm_desktop_reply(ewmh, ck, &d, nullptr)) continue;
            if (d != ALL_DESKTOPS && d != static_cast<uint32_t>(curr_desktop)) continue;
        

            auto wn = xcb_ewmh_get_wm_name(ewmh, client_list.windows[i]);
            xcb_ewmh_get_utf8_strings_reply_t name_reply;
            if (!xcb_ewmh_get_wm_name_reply(ewmh, wn, &name_reply, nullptr)) continue;
            std::string window_title(reinterpret_cast<const char*>(name_reply.strings), name_reply.strings_len);
            xcb_ewmh_get_utf8_strings_reply_wipe(&name_reply);
            toml::table t;
            std::cout << window_title << std::endl;
            t.insert("title", window_title);
            t.insert("desktop", static_cast<int64_t>(d));
        

            auto geom_cookie = xcb_get_geometry(c, client_list.windows[i]);
            auto *wgem = xcb_get_geometry_reply(c, geom_cookie, nullptr);

            if (!wgem) continue;

            auto tck = xcb_translate_coordinates(c, client_list.windows[i], wgem->root, 0, 0);
            auto trep = xcb_translate_coordinates_reply(c, tck, nullptr); 

            if (!trep) { free(wgem); continue; }

            xcb_ewmh_get_extents_reply_t fx{};
            get_frame_extents(ewmh, client_list.windows[i], fx);
            if (wgem) { 

                t.insert("geom", GEOM_TO_TOML(wgem));

                int frame_x = trep->dst_x - fx.left;
                int frame_y = trep->dst_y - fx.top;
                int frame_w = wgem->width + fx.left + fx.right;
                int frame_h = wgem->height + fx.top + fx.bottom;

                xcb_get_geometry_reply_t frame_geom = *wgem;
                frame_geom.x = frame_x;
                frame_geom.y = frame_y;
                frame_geom.width = frame_w;
                frame_geom.height = frame_h;

                t.insert("geom_abs", NGEOM_TO_TOML(frame_geom));

                auto* mon = pick_monitor(monitors, frame_x, frame_y, frame_w, frame_h);
                if (mon) t.insert("monitor", mon->name);
            }
            windows.push_back(std::move(t));
        }
        toml::table root;
        root.insert("window", std::move(windows));
        std::ofstream out("scene.toml"); out << root;
    }
    xcb_ewmh_get_windows_reply_wipe(&client_list);
    return true;
}

int main() {
    std::cout << "Connecting" << std::endl;
    xcb_connection_t *c = xcb_connect(nullptr, &SCREEN_N);
    std::cout << "Checking" << std::endl;
    if (xcb_connection_has_error(c)) return 1;
    std::cout << "Connected" << std::endl;
    xcb_screen_t *s = xcb_setup_roots_iterator(xcb_get_setup(c)).data;

    xcb_ewmh_connection_t ewmh;
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(c, &ewmh);
    if (!xcb_ewmh_init_atoms_replies(&ewmh, cookie, nullptr)) {
        std::cerr << "Failed to initialize EWMH atoms" << std::endl;
        return 1;
    }
    fmt::print("Capturing desktop");
    capture_desktop(c, &ewmh, s);
    xcb_disconnect(c);
    return 0;
}

