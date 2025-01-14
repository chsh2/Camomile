/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <limits>
#include <cmath>

#include "PdGui.hpp"
#include "PdInstance.hpp"
#include "../PluginConfig.h"

extern "C"
{
#include <z_libpd.h>
#include <m_imp.h>
#include <g_canvas.h>
#include <g_all_guis.h>
#include "x_libpd_extra_utils.h"
}

namespace pd
{
    // ==================================================================================== //
    //                                      GUI                                             //
    // ==================================================================================== //
    
    // False GATOM
    typedef struct _fake_gatom
    {
        t_text a_text;
        int a_flavor;           /* A_FLOAT, A_SYMBOL, or A_LIST */
        t_glist *a_glist;       /* owning glist */
        t_float a_toggle;       /* value to toggle to */
        t_float a_draghi;       /* high end of drag range */
        t_float a_draglo;       /* low end of drag range */
        t_symbol *a_label;      /* symbol to show as label next to box */
        t_symbol *a_symfrom;    /* "receive" name -- bind ourselves to this */
        t_symbol *a_symto;      /* "send" name -- send to this on output */
        t_binbuf *a_revertbuf;  /* binbuf to revert to if typing canceled */
        int a_dragindex;        /* index of atom being dragged */
        int a_fontsize;
        unsigned int a_shift:1;         /* was shift key down when drag started? */
        unsigned int a_wherelabel:2;    /* 0-3 for left, right, above, below */
        unsigned int a_grabbed:1;       /* 1 if we've grabbed keyboard */
        unsigned int a_doubleclicked:1; /* 1 if dragging from a double click */
        t_symbol *a_expanded_to;
    } t_fake_gatom;
    
    static t_atom *fake_gatom_getatom(t_fake_gatom *x)
    {
        int ac = binbuf_getnatom(x->a_text.te_binbuf);
        t_atom *av = binbuf_getvec(x->a_text.te_binbuf);
        if (x->a_flavor == A_FLOAT && (ac != 1 || av[0].a_type != A_FLOAT))
        {
            binbuf_clear(x->a_text.te_binbuf);
            binbuf_addv(x->a_text.te_binbuf, "f", 0.);
        }
        else if (x->a_flavor == A_SYMBOL && (ac != 1 || av[0].a_type != A_SYMBOL))
        {
            binbuf_clear(x->a_text.te_binbuf);
            binbuf_addv(x->a_text.te_binbuf, "s", &s_);
        }
        return (binbuf_getvec(x->a_text.te_binbuf));
    }

    Gui::Gui(void* ptr, void* patch, Instance* instance) noexcept :
    Object(ptr, patch, instance), m_type(Type::Undefined)
    {
        const std::string name = getName();
        if(name == "bng")
        {
           m_type = Type::Bang;
        }
        else if(name == "hsl")
        {
           m_type = Type::HorizontalSlider;
        }
        else if(name == "vsl")
        {
           m_type = Type::VerticalSlider;
        }
        else if(name == "tgl")
        {
           m_type = Type::Toggle;
        }
        else if(name == "nbx")
        {
           m_type = Type::Number;
        }
        else if(name == "vradio")
        {
           m_type = Type::VerticalRadio;
        }
        else if(name == "hradio")
        {
           m_type = Type::HorizontalRadio;
        }
        else if(name == "cnv")
        {
           m_type = Type::Panel;
        }
        else if(name == "vu")
        {
           m_type = Type::VuMeter;
        }
        else if(name == "text")
        {
           m_type = Type::Comment;
        }
        else if(name == "gatom")
        {
            if(static_cast<t_fake_gatom*>(m_ptr)->a_flavor == A_FLOAT)
                m_type = Type::AtomNumber;
            else if(static_cast<t_fake_gatom*>(m_ptr)->a_flavor == A_SYMBOL)
                m_type = Type::AtomSymbol;
            else if(static_cast<t_fake_gatom*>(m_ptr)->a_flavor == A_NULL)
                m_type = Type::AtomList;
        }
        else if(name == "canvas")
        {
            if(static_cast<t_canvas*>(m_ptr)->gl_list)
            {
                t_class* c = static_cast<t_canvas*>(m_ptr)->gl_list->g_pd;
                if(c && c->c_name && std::string(c->c_name->s_name) == std::string("array"))
                {
                    m_type = Type::Array;
                }
            }
            if(m_type != Type::Array && static_cast<t_canvas*>(m_ptr)->gl_isgraph)
            {
                m_type = Type::GraphOnParent;
                canvas_vis(static_cast<t_canvas*>(m_ptr), 1.f);
            }
        }
    }
    
    size_t Gui::getNumberOfSteps() const noexcept
    {
        if(!m_ptr)
            return 0.f;
        if(m_type == Type::Toggle)
        {
            return 2;
        }
        else if(m_type == Type::HorizontalRadio)
        {
            return (static_cast<t_hdial*>(m_ptr))->x_number - 1;
        }
        else if(m_type == Type::VerticalRadio)
        {
            return (static_cast<t_vdial*>(m_ptr))->x_number;
        }
        else if(m_type == Type::AtomNumber)
        {
            return static_cast<t_text*>(m_ptr)->te_width == 1;
        }
        return 0.f;
    }
    
    float Gui::getMinimum() const noexcept
    {
        if(!m_ptr)
            return 0.f;
        if(m_type == Type::HorizontalSlider)
        {
            return (static_cast<t_hslider*>(m_ptr))->x_min;
        }
        else if(m_type == Type::VerticalSlider)
        {
            return (static_cast<t_vslider*>(m_ptr))->x_min;
        }
        else if(m_type == Type::Number)
        {
            return (static_cast<t_my_numbox*>(m_ptr))->x_min;
        }
        else if(m_type == Type::AtomNumber)
        {
            t_fake_gatom const* gatom = static_cast<t_fake_gatom const*>(m_ptr);
            if(std::abs(gatom->a_draglo) > std::numeric_limits<float>::epsilon() ||
               std::abs(gatom->a_draghi) > std::numeric_limits<float>::epsilon())
            {
                return gatom->a_draglo;
            }
            return -std::numeric_limits<float>::max();
        }
        return 0.f;
    }
    
    float Gui::getMaximum() const noexcept
    {
        if(!m_ptr)
            return 1.f;
        if(m_type == Type::HorizontalSlider)
        {
            return (static_cast<t_hslider*>(m_ptr))->x_max;
        }
        else if(m_type == Type::VerticalSlider)
        {
            return (static_cast<t_vslider*>(m_ptr))->x_max;
        }
        else if(m_type == Type::Number)
        {
            return (static_cast<t_my_numbox*>(m_ptr))->x_max;
        }
        else if(m_type == Type::HorizontalRadio)
        {
            return (static_cast<t_hdial*>(m_ptr))->x_number - 1;
        }
        else if(m_type == Type::VerticalRadio)
        {
            return (static_cast<t_vdial*>(m_ptr))->x_number - 1;
        }
        else if(m_type == Type::Bang)
        {
            return 1;
        }
        else if(m_type == Type::AtomNumber)
        {
            t_fake_gatom const* gatom = static_cast<t_fake_gatom const*>(m_ptr);
            if(std::abs(gatom->a_draglo) > std::numeric_limits<float>::epsilon() ||
               std::abs(gatom->a_draghi) > std::numeric_limits<float>::epsilon())
            {
                return gatom->a_draghi;
            }
            return std::numeric_limits<float>::max();
        }
        return 1.f;
    }
    
    float Gui::getValue() const noexcept
    {
        if(!m_ptr)
            return 0.f;
        if(m_type == Type::HorizontalSlider)
        {
            return (static_cast<t_hslider*>(m_ptr))->x_fval;
        }
        else if(m_type == Type::VerticalSlider)
        {
            return (static_cast<t_vslider*>(m_ptr))->x_fval;
        }
        else if(m_type == Type::Toggle)
        {
            return (static_cast<t_toggle*>(m_ptr))->x_on;
        }
        else if(m_type == Type::Number)
        {
            return (static_cast<t_my_numbox*>(m_ptr))->x_val;
        }
        else if(m_type == Type::HorizontalRadio)
        {
            return (static_cast<t_hdial*>(m_ptr))->x_on;
        }
        else if(m_type == Type::VerticalRadio)
        {
            return (static_cast<t_vdial*>(m_ptr))->x_on;
        }
        else if(m_type == Type::Bang)
        {
            // hack to trigger off the bang if no GUI update
            if((static_cast<t_bng*>(m_ptr))->x_flashed > 0)
            {
                static_cast<t_bng*>(m_ptr)->x_flashed = 0;
                return 1.0f;
            }
            return 0.0f;
        }
        else if(m_type == Type::VuMeter)
        {
            return 0;
        }
        else if(m_type == Type::AtomNumber)
        {
            return atom_getfloat(fake_gatom_getatom(static_cast<t_fake_gatom*>(m_ptr)));
        }
        return 0.f;
    }
    
    void Gui::setValue(float value) noexcept
    {
        if(!m_ptr || m_type == Type::Comment || m_type == Type::AtomSymbol)
            return;
        m_instance->enqueueDirectMessages(m_ptr, value);
    }
    
    bool Gui::jumpOnClick() const noexcept
    {
        if(!m_ptr)
            return 0.f;
        if(m_type == Type::HorizontalSlider)
        {
            return (static_cast<t_hslider*>(m_ptr))->x_steady == 0;
        }
        else if(m_type == Type::VerticalSlider)
        {
            return (static_cast<t_vslider*>(m_ptr))->x_steady == 0;
        }
        return false;
    }
    
    bool Gui::isLogScale() const noexcept
    {
        if(!m_ptr)
            return 0.f;
        if(m_type == Type::HorizontalSlider)
        {
            return (static_cast<t_hslider*>(m_ptr))->x_lin0_log1 != 0;
        }
        else if(m_type == Type::VerticalSlider)
        {
            return (static_cast<t_vslider*>(m_ptr))->x_lin0_log1 != 0;
        }
        return false;
    }
    
    std::string Gui::getSymbol() const noexcept
    {
        if(!m_ptr || m_type != Type::AtomSymbol)
            return std::string();
        else
        {
            m_instance->setThis();
            return atom_getsymbol(fake_gatom_getatom(static_cast<t_fake_gatom*>(m_ptr)))->s_name;
        }
    }
    
    void Gui::setSymbol(std::string const& value) noexcept
    {
        if(!m_ptr || m_type != Type::AtomSymbol)
            return;
        m_instance->enqueueDirectMessages(m_ptr, value);
    }
    
    std::vector<Atom> Gui::getList() const noexcept
    {
        if(!m_ptr || m_type != Type::AtomList)
            return {};
        else
        {
            std::vector<Atom> array;
            m_instance->setThis();
            
            int ac = binbuf_getnatom(static_cast<t_fake_gatom*>(m_ptr)->a_text.te_binbuf);
            t_atom *av = binbuf_getvec(static_cast<t_fake_gatom*>(m_ptr)->a_text.te_binbuf);
            array.reserve(ac);
            for(int i = 0; i < ac; ++i)
            {
                if(av[i].a_type == A_FLOAT)
                {
                    array.push_back({atom_getfloat(av+i)});
                }
                else if(av[i].a_type == A_SYMBOL)
                {
                    array.push_back({atom_getsymbol(av+i)->s_name});
                }
                else
                {
                    array.push_back({});
                }
            }
            return array;
        }
    }
    
    void Gui::setList(std::vector<Atom> const& value) noexcept
    {
        if(!m_ptr || m_type != Type::AtomList)
            return;
        m_instance->enqueueDirectMessages(m_ptr, value);
    }
    
    float Gui::getFontHeight() const noexcept
    {
        if(!m_ptr )
            return 0;
        if(isIEM())
        {
            return static_cast<float>((static_cast<t_iemgui*>(m_ptr))->x_fontsize);
        }
        else
        {
            return libpd_get_canvas_font_height(static_cast<t_canvas*>(m_patch));
        }
    }
    
    std::string Gui::getFontName() const
    {
        if(!m_ptr )
            return std::string(sys_font);
        if(isIEM())
        {
            return std::string((static_cast<t_iemgui*>(m_ptr))->x_font);
        }
        else
        {
            return std::string(sys_font);
        }
    }
    
    static unsigned int fromIemColors(int const color)
    {
        unsigned int const c = static_cast<unsigned int>(color << 8 | 0xFF);
        return ((0xFF << 24) | ((c >> 24) << 16) | ((c >> 16) << 8) | (c >> 8));
    }
    
    unsigned int Gui::getBackgroundColor() const noexcept
    {
        uint32_t color = CamomileEnvironment::getDefaultBackgroundColor();
        if(m_ptr && isIEM())
        {
            color = libpd_iemgui_get_background_color(m_ptr);
        }
        if(color == CamomileEnvironment::getTransparentColor()) 
        {
            color %= 0xFF000000;
        }
        return color;
    }
    
    unsigned int Gui::getForegroundColor() const noexcept
    {
        if(m_ptr && isIEM())
        {
            return libpd_iemgui_get_foreground_color(m_ptr);
        }
        return CamomileEnvironment::getDefaultForegroundColor();
    }
    
    std::array<int, 4> Gui::getBounds() const noexcept
    {
        if(m_type == Type::Panel)
        {
            std::array<int, 4> const bounds = Object::getBounds();
            return {bounds[0], bounds[1],
                static_cast<t_my_canvas*>(m_ptr)->x_vis_w + 1,
                static_cast<t_my_canvas*>(m_ptr)->x_vis_h + 1};
        }
        else if(m_type == Type::AtomNumber || m_type == Type::AtomSymbol || m_type == Type::AtomList)
        {
            std::array<int, 4> const bounds = Object::getBounds();
            return {bounds[0], bounds[1], bounds[2], bounds[3] - 2};
        }
        else if(m_type == Type::Comment)
        {
            std::array<int, 4> const bounds = Object::getBounds();
            return {bounds[0] + 2, bounds[1] + 2, bounds[2], bounds[3] - 2};
        }
        return Object::getBounds();
    }
    
    Array Gui::getArray() const noexcept
    {
        if(m_type == Type::Array)
        {
            return m_instance->getArray(libpd_array_get_name(static_cast<t_canvas*>(m_ptr)->gl_list));
        }
        return Array();
    }
    
    Patch Gui::getPatch() const noexcept
    {
        if(m_type == Type::GraphOnParent)
        {
            return Patch(m_ptr, m_instance);
        }
        return Patch();
    }
    
    // ==================================================================================== //
    //                                      LABEL                                           //
    // ==================================================================================== //
    
    Label Gui::getLabel() const noexcept
    {
        m_instance->setThis();
        if(isIEM())
        {
            t_symbol const* sym = canvas_realizedollar(static_cast<t_iemgui*>(m_ptr)->x_glist, static_cast<t_iemgui*>(m_ptr)->x_lab);
            if(sym)
            {
                auto const text = std::string(sym->s_name);
                if(!text.empty() && text != std::string("empty"))
                {
                    auto const* iemgui  = static_cast<t_iemgui*>(m_ptr);
                    auto const color    = fromIemColors(iemgui->x_lcol);
                    auto const bounds   = getBounds();
                    auto const posx     = bounds[0] + iemgui->x_ldx;
                    auto const posy     = bounds[1] + iemgui->x_ldy;
                    auto const fontname = getFontName();
                    auto const fontheight = getFontHeight();
                    return Label(text, color, posx, posy, fontname, fontheight);
                }
            }
        }
        else if(isAtom())
        {
            t_symbol const* sym = canvas_realizedollar(static_cast<t_fake_gatom*>(m_ptr)->a_glist, static_cast<t_fake_gatom*>(m_ptr)->a_label);
            if(sym)
            {
                auto const text         = std::string(sym->s_name);
                auto const bounds       = getBounds();
                auto const* gatom       = static_cast<t_fake_gatom*>(m_ptr);
                auto const color        = 0xff000000;
                auto const fontname     = getFontName();
                auto const fontheight   = sys_hostfontsize(glist_getfont(static_cast<t_canvas*>(m_patch)), glist_getzoom(static_cast<t_canvas*>(m_patch)));
                
                if (gatom->a_wherelabel == 0) // Left
                {
                    auto const nchars   = static_cast<int>(text.size());
                    auto const fwidth   = glist_fontwidth(static_cast<t_fake_gatom*>(m_ptr)->a_glist);
                    auto const posx     = bounds[0] - 4 - nchars * fwidth;
                    auto const posy     = bounds[1] + 2 + fontheight / 2;
                    return Label(text, color,posx, posy, fontname, fontheight);
                }
                else if (gatom->a_wherelabel == 1) // Right
                {
                    auto const posx     = bounds[0] + bounds[2] + 2;
                    auto const posy     = bounds[1] + 2 + fontheight / 2;
                    return Label(text, color, posx, posy, fontname, fontheight);
                }
                else if (gatom->a_wherelabel == 2) // Up
                {
                    auto const posx     = bounds[0] - 1;
                    auto const posy     = bounds[1] - 1 - fontheight / 2;
                    return Label(text, color, posx, posy, fontname, fontheight);
                }
                auto const posx     = bounds[0] - 1;
                auto const posy     = bounds[1] + bounds[3] + 2 + fontheight / 2;
                return Label(text, color, posx, posy, fontname, fontheight); // Down
            }
        }
        return Label();
    }
    
    Label::Label() noexcept :
    m_text(""),
    m_color(0xff000000),
    m_position({0, 0})
    {
    }
    
    Label::Label(Label const& other) noexcept :
    m_text(other.m_text),
    m_color(other.m_color),
    m_position(other.m_position)
    {
    }
    
    Label::Label(std::string const& text, unsigned int color, int x, int y, std::string const& fontname, float fontheight) noexcept :
    m_text(text),
    m_color(color),
    m_position({x, y}),
    m_font_name(fontname),
    m_font_height(fontheight)
    {
        
    }
    
    float Label::getFontHeight() const noexcept
    {
        return m_font_height;
    }
    
    std::string Label::getFontName() const
    {
        return m_font_name;
    }
}



