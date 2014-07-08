#ifndef _SUNGUI_H_
#define _SUNGUI_H_

#include <string>
#include <memory>
#include <vector>
#include <list>
#include <set>
#include <unordered_map>
#include <functional>

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

namespace sgui
{
    
    struct SGVertexVT {
        float vertex[3] = {0.0f, 0.0f, 0.0f};
        float texcoord[2] = {0.0f, 0.0f};
        static const int tex_offset = 12;
    };
    
    struct SGVertexVCT {
        float vertex[3] = {0.0f, 0.0f, 0.0f};
        unsigned int color = 0xffffffff;
        float texcoord[2] = {0.0f, 0.0f};
        static const int color_offset = 12;
        static const int tex_offset = 16;
    };
    
    struct SGVertexVNT {
        float vertex[3] = {0.0f, 0.0f, 0.0f};
        float normal[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        float texcoord[2] = {0.0f, 0.0f};
        static const int normal_offset = 12;
        static const int tex_offset = 28;
    };
    
    struct SGVertexVNCT {
        float vertex[3] = {0.0f, 0.0f, 0.0f};
        float normal[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        unsigned int color = 0xffffffff;
        float texcoord[2] = {0.0f, 0.0f};
        static const int normal_offset = 12;
        static const int color_offset = 28;
        static const int tex_offset = 32;
    };
    
    // ===== Delegate Implement =====
    // OT: Object Type
    // ST: Sender Type
    // ET: Event Type
    // MT: Method Type
    // FT: Lambda Type
    
    template<typename ST, typename ET = void>
    class SGDelegate {
    public:
        virtual bool Invoke(ST& sender, ET evt) = 0;
    };
    
    template<typename ST>
    class SGDelegate<ST, void> {
    public:
        virtual bool Invoke(ST& sender) = 0;
    };
    
    template<typename OT, typename ST, typename ET = void>
    class SGDelegateObject : public SGDelegate<ST, ET> {
        typedef bool (OT::*MT)(ST&, ET);
        
    public:
        SGDelegateObject(OT* obj, MT fun): invoke_object(obj), invoke_method(fun) {}
        virtual bool Invoke(ST& sender, ET evt) {
            return (invoke_object->*invoke_method)(sender, evt);
        }
        
    protected:
        OT* invoke_object = nullptr;
        MT invoke_method = nullptr;
    };
    
    template<typename OT, typename ST>
    class SGDelegateObject<OT, ST, void> : public SGDelegate<ST, void> {
        typedef bool (OT::*MT)(ST&);
        
    public:
        SGDelegateObject(OT* obj, MT fun): invoke_object(obj), invoke_method(fun) {}
        virtual bool Invoke(ST& sender) {
            return (invoke_object->*invoke_method)(sender);
        }
        
    protected:
        OT* invoke_object = nullptr;
        MT invoke_method = nullptr;
    };
    
    template<typename ST, typename ET = void>
    class SGDelegateStdFunction : public SGDelegate<ST, ET> {
        
    public:
        SGDelegateStdFunction(const std::function<bool (ST&, ET)>& fun): invoke_method(fun) {}
        virtual bool Invoke(ST& sender, ET evt) {
            return invoke_method(sender, evt);
        }
        
    protected:
        std::function<bool (ST&, ET)> invoke_method = nullptr;
    };
    
    template<typename ST>
    class SGDelegateStdFunction<ST, void> : public SGDelegate<ST, void> {
        
    public:
        SGDelegateStdFunction(const std::function<bool (ST&)>& fun): invoke_method(fun) {}
        virtual bool Invoke(ST& sender) {
            return invoke_method(sender);
        }
        
    protected:
        std::function<bool (ST&)> invoke_method = nullptr;
    };
    
    template<typename ST, typename ET = void>
    class SGEventHandler {
    public:
        template<typename OT>
        void Bind(OT* obj, bool (OT::*fun)(ST&, ET)) {
            delegate_ptrs.push_back(std::make_shared<SGDelegateObject<OT, ST, ET>>(obj, fun));
        }
        
        template<typename FT>
        void Bind(FT f) {
            auto ptr = std::make_shared<SGDelegateStdFunction<ST, ET>>(std::function<bool (ST&, ET)>(f));
            delegate_ptrs.push_back(ptr);
        }
        
        bool TriggerEvent(ST& sender, ET evt) {
            if(delegate_ptrs.size() == 0)
                return false;
            bool ret;
            for(auto& ptr : delegate_ptrs)
                ret = ptr->Invoke(sender, evt) || ret;
            return ret;
        }
        
    protected:
        std::vector<std::shared_ptr<SGDelegate<ST, ET>>> delegate_ptrs;
    };
    
    template<typename ST>
    class SGEventHandler<ST, void> {
    public:
        template<typename OT>
        void Bind(OT* obj, bool (OT::*fun)(ST&)) {
            delegate_ptrs.push_back(std::make_shared<SGDelegateObject<OT, ST, void>>(obj, fun));
        }
        
        template<typename FT>
        void Bind(FT f) {
            auto ptr = std::make_shared<SGDelegateStdFunction<ST, void>>(std::function<bool (ST&)>(f));
            delegate_ptrs.push_back(ptr);
        }
        
        bool TriggerEvent(ST& sender) {
            if(delegate_ptrs.size() == 0)
                return false;
            bool ret;
            for(auto& ptr : delegate_ptrs)
                ret = ret || ptr->Invoke(sender);
            return ret;
        }
        
    protected:
        std::vector<std::shared_ptr<SGDelegate<ST, void>>> delegate_ptrs;
    };
    
    // ===== Delegate Implement End =====
    
    typedef sf::Vector2f v2f;
    typedef sf::Vector2i v2i;
    
    // ===== GUI Components =====
    
    class SGWidgetContainer;
    class SGGUIRoot;
    
    class SGWidget : public std::enable_shared_from_this<SGWidget> {
        friend class SGWidgetContainer;
        friend class SGGUIRoot;
        
    public:
        virtual ~SGWidget() {}
        
        virtual bool CheckInside(v2i pos);
        virtual void UpdateVertices() = 0;
        virtual void Draw() = 0;
        virtual void PostRedraw();
        virtual void PostResize(bool res, bool rep);
        virtual bool AllowFocus() const { return true; }
        virtual std::shared_ptr<SGWidget> GetDragingTarget() { return nullptr; }
        void SetPosition(v2i pos, v2f = v2f(0.0f, 0.0f));
        void SetSize(v2i sz, v2f = v2f(0.0f, 0.0f));
        void SetX(int x, float propx = 0.0f);
        void SetY(int y, float propy = 0.0f);
        void SetWidth(int w, float propw = 0.0f);
        void SetHeight(int h, float proph = 0.0f);
        void SetColor(unsigned int c);
        v2i GetPosition() const { return position_abs; }
        v2i GetSize() const { return size_abs; }
        void SetVisible(bool v);
        bool IsVisible() const;
        void SetCustomValue(unsigned int v) { cvalue = v; }
        unsigned int GetCustomValue() const { return cvalue; }
        void SetCustomObject(void* v) { cobject = v; }
        void* GetCustomObject() const { return cobject; }
        
        SGEventHandler<SGWidget, sf::Event::MouseMoveEvent> eventMouseMove;
        SGEventHandler<SGWidget> eventMouseEnter;
        SGEventHandler<SGWidget> eventMouseLeave;
        SGEventHandler<SGWidget, sf::Event::MouseButtonEvent> eventMouseButtonDown;
        SGEventHandler<SGWidget, sf::Event::MouseButtonEvent> eventMouseButtonUp;
        SGEventHandler<SGWidget, sf::Event::MouseWheelEvent> eventMouseWheel;
        SGEventHandler<SGWidget, sf::Event::KeyEvent> eventKeyDown;
        SGEventHandler<SGWidget, sf::Event::KeyEvent> eventKeyUp;
        SGEventHandler<SGWidget, sf::Event::TextEvent> eventCharEnter;
        SGEventHandler<SGWidget> eventGetFocus;
        SGEventHandler<SGWidget> eventLostFocus;
        
        SGEventHandler<SGWidget, v2i> eventDragBegin;
        SGEventHandler<SGWidget, v2i> eventDragEnd;
        SGEventHandler<SGWidget, v2i> eventDragUpdate;
        
    protected:
        virtual bool EventMouseMove(sf::Event::MouseMoveEvent evt);
        virtual bool EventMouseEnter();
        virtual bool EventMouseLeave();
        virtual bool EventMouseButtonDown(sf::Event::MouseButtonEvent evt);
        virtual bool EventMouseButtonUp(sf::Event::MouseButtonEvent evt);
        virtual bool EventMouseWheel(sf::Event::MouseWheelEvent evt);
        virtual bool EventKeyDown(sf::Event::KeyEvent evt);
        virtual bool EventKeyUp(sf::Event::KeyEvent evt);
        virtual bool EventCharEnter(sf::Event::TextEvent evt);
        virtual bool EventGetFocus();
        virtual bool EventLostFocus();
        
        virtual bool DragingBegin(v2i evt);
        virtual bool DragingEnd(v2i evt);
        virtual bool DragingUpdate(v2i evt);
        
    protected:
        v2i position;
        v2i position_abs;
        v2f position_prop;
        v2i size;
        v2i size_abs;
        v2f size_prop;
        v2i position_drag;
        bool visible = true;
        bool enabled = true;
        unsigned int cvalue = 0;
        unsigned int color = 0xffffffff;
        void* cobject = nullptr;
        std::weak_ptr<SGWidgetContainer> parent;
        bool vertices_dirty = true;
        int buffer_index = 0;
    };
    
    class SGTextBase {
    public:
        SGTextBase();
        virtual ~SGTextBase();
        virtual void SetFont(sf::Font* ft, unsigned int sz);
        virtual void SetSpacing(unsigned int x, unsigned int y);
        virtual int GetLineSpacing();
        virtual v2i GetTextOffset() = 0;
        virtual int GetMaxWidth() = 0;
        virtual bool IsMultiLine() = 0;
        virtual void ClearText();
        virtual unsigned int GetTextColor(int index);
        virtual void SetText(const std::wstring& t, unsigned int cl);
        virtual void AppendText(const std::wstring& t, unsigned int cl);
        virtual void EvaluateSize(const std::wstring& t = L"");
        virtual void UpdateTextVertex();
        virtual void DrawText();
        virtual std::wstring GetText() { return text; }
        
    protected:
        bool text_update = true;
        short mem_size = 0;
        unsigned int tbo[2] = {0, 0};
        sf::Font* font = nullptr;
        unsigned int font_size = 0;
        unsigned int spacing_x = 1;
        unsigned int spacing_y = 1;
        unsigned int vert_size = 0;
        v2i tex_size;
        int text_width = 0;
        int text_height = 0;
        int text_width_cur = 0;
        std::wstring text;
        std::vector<unsigned int> color_vec;
        std::vector<v2i> text_pos_array;
    };
    
    class SGSpriteBase {
    public:
        virtual ~SGSpriteBase();
        virtual void SetImage(sf::Texture* img, sf::IntRect varea);
        virtual void AddTexRect(sf::IntRect tarea);
        virtual void SetImage(sf::Texture* img, std::vector<v2i>& verts);
        virtual void AddTexcoord(std::vector<v2i>& texcoords);
        virtual v2i GetImageOffset() = 0;
        void SetFrameTime(float ft) { frame_time = ft; }
        virtual void UpdateImage();
        virtual void DrawImage();
        
    protected:
        unsigned int imgvbo[2] = {0, 0};
        bool img_update = true;
        bool img_dirty = true;
        float frame_time = 0.016f;
        sf::Clock frame_clock;
        sf::Texture* img_texture = nullptr;
        v2i img_offset = {0, 0};
        std::vector<v2i> verts;
        std::vector<std::vector<v2i>> texcoords;
    };
    
    class SGWidgetContainer : public SGWidget {
        friend class SGWidget;
        
    public:
        virtual ~SGWidgetContainer();
        virtual void PostResize(bool res, bool rep);
        
        virtual std::shared_ptr<SGWidget> GetHovingWidget(v2i pos);
        virtual void AddChild(std::shared_ptr<SGWidget> chd);
        virtual void RemoveChild(std::shared_ptr<SGWidget> chd);
        
    protected:
        virtual bool EventMouseMove(sf::Event::MouseMoveEvent evt);
        virtual bool EventMouseEnter();
        virtual bool EventMouseLeave();
        virtual bool EventMouseButtonDown(sf::Event::MouseButtonEvent evt);
        virtual bool EventMouseButtonUp(sf::Event::MouseButtonEvent evt);
        virtual bool EventMouseWheel(sf::Event::MouseWheelEvent evt);
        virtual bool EventKeyDown(sf::Event::KeyEvent evt);
        virtual bool EventKeyUp(sf::Event::KeyEvent evt);
        virtual bool EventCharEnter(sf::Event::TextEvent evt);
        
    protected:
        std::weak_ptr<SGWidget> hoving;
        std::weak_ptr<SGWidget> focus_widget;
        std::list<std::shared_ptr<SGWidget>> children;
    };
    
    struct SGConfig {
        std::unordered_map<std::string, int> int_config;
        std::unordered_map<std::string, std::string> string_config;
        std::unordered_map<std::string, sf::Rect<int>> tex_config;
        std::unordered_map<std::string, std::vector<v2i>> coord_config;
    };
    
    class SGGUIRoot : public SGWidgetContainer {
    public:
        virtual ~SGGUIRoot();
        
        void SetSceneSize(v2i size);
        v2i GetSceneSize();
        void ConvertXY(int x, int y, float v[]) {
            v[0] = (float)x / scene_size.x * 2.0f - 1.0f;
            v[1] = 1.0f - (float)y / scene_size.y * 2.0f;
        }
        void TexUV(int u, int v, float t[]) {
            t[0] = (float)u / tex_size.x;
            t[1] = (float)v / tex_size.y;
        }
        template<typename T>
        void SetRectVertex(T* v, int x, int y, int w, int h, sf::IntRect uv) {
            v[0].vertex[0] = (float)x / scene_size.x * 2.0f - 1.0f;
            v[0].vertex[1] = 1.0f - (float)y / scene_size.y * 2.0f;
            v[1].vertex[0] = (float)(x + w) / scene_size.x * 2.0f - 1.0f;
            v[1].vertex[1] = 1.0f - (float)y / scene_size.y * 2.0f;
            v[2].vertex[0] = (float)x / scene_size.x * 2.0f - 1.0f;
            v[2].vertex[1] = 1.0f - (float)(y + h) / scene_size.y * 2.0f;
            v[3].vertex[0] = (float)(x + w) / scene_size.x * 2.0f - 1.0f;
            v[3].vertex[1] = 1.0f - (float)(y + h) / scene_size.y * 2.0f;
            v[0].texcoord[0] = (float)uv.left / tex_size.x;
            v[0].texcoord[1] = (float)uv.top / tex_size.y;
            v[1].texcoord[0] = (float)(uv.left + uv.width) / tex_size.x;
            v[1].texcoord[1] = (float)uv.top / tex_size.y;
            v[2].texcoord[0] = (float)uv.left / tex_size.x;
            v[2].texcoord[1] = (float)(uv.top + uv.height) / tex_size.y;
            v[3].texcoord[0] = (float)(uv.left + uv.width) / tex_size.x;
            v[3].texcoord[1] = (float)(uv.top + uv.height) / tex_size.y;
        }
        
        void BindGuiTexture() {
            if(cur_texture != &gui_texture) {
                sf::Texture::bind(&gui_texture);
                cur_texture = &gui_texture;
            }
        }
        
        void BindFontTexture(unsigned int sz) {
            auto& tex = gui_font.getTexture(sz);
            if(cur_texture != &tex) {
                sf::Texture::bind(&tex);
                cur_texture = (sf::Texture*)&tex;
            }
        }
        
        void BindTexture(sf::Texture* t = nullptr) {
            if(t != cur_texture) {
                sf::Texture::bind(t);
                cur_texture = t;
            }
        }
        
        void BeginScissor(sf::IntRect rt);
        void EndScissor();
        
        virtual bool CheckInside(v2i pos);
        virtual std::shared_ptr<SGWidget> GetHovingWidget(v2i pos);
        virtual void UpdateVertices();
        virtual void Draw();
        void ObjectDragingBegin(std::shared_ptr<SGWidget> d, sf::Event::MouseMoveEvent evt);
        void ObjectDragingEnd(sf::Event::MouseMoveEvent evt);
        void SetClickingObject(std::shared_ptr<SGWidget> ptr) { clicking_object = ptr; }
        std::shared_ptr<SGWidget> GetClickObject() { return clicking_object.lock(); }
        void LoadConfigs();
        void AddConfig(const std::string& wtype, SGConfig& conf) { configs[wtype] = &conf; }
        sf::Font& GetGUIFont() { return gui_font; }
        sf::Texture& GetGUITexture() { return gui_texture; }
        unsigned int GetDefaultInt(const std::string& key) { return basic_config.int_config[key]; }
        
        bool InjectMouseMoveEvent(sf::Event::MouseMoveEvent evt);
        bool InjectMouseEnterEvent();
        bool InjectMouseLeaveEvent();
        bool InjectMouseButtonDownEvent(sf::Event::MouseButtonEvent evt);
        bool InjectMouseButtonUpEvent(sf::Event::MouseButtonEvent evt);
        bool InjectMouseWheelEvent(sf::Event::MouseWheelEvent evt);
        bool InjectKeyDownEvent(sf::Event::KeyEvent evt);
        bool InjectKeyUpEvent(sf::Event::KeyEvent evt);
        bool InjectCharEvent(sf::Event::TextEvent evt);
        
        static SGGUIRoot& GetSingleton();
        
    protected:
        bool inside_scene = false;
        v2i scene_size;
        v2i tex_size;
        std::weak_ptr<SGWidget> draging_object;
        std::weak_ptr<SGWidget> clicking_object;
        sf::Font gui_font;
        sf::Texture gui_texture;
        sf::Texture* cur_texture = nullptr;
        std::unordered_map<std::string, SGConfig*> configs;
        std::list<sf::IntRect> scissor_stack;
        
        static SGConfig basic_config;
    };
    
    class SGWindow : public SGWidgetContainer, public SGTextBase {
    public:
        virtual ~SGWindow();
        virtual void UpdateVertices();
        virtual void Draw();
        virtual std::shared_ptr<SGWidget> GetDragingTarget() { return shared_from_this(); }
        virtual v2i GetTextOffset();
        virtual int GetMaxWidth();
        virtual bool IsMultiLine();
        void Destroy();
        std::shared_ptr<SGWidget> GetCloseButton() { return children.front(); }
        void SetTitle(const std::wstring& t);
        
    protected:
        virtual bool DragingBegin(v2i evt);
        virtual bool DragingUpdate(v2i evt);
        bool CloseButtonClick(SGWidget& sender);
        unsigned int vbo[2] = {0, 0};
        unsigned int drag_type = 0;
        v2i drag_diff;
        
    public:
        static std::shared_ptr<SGWindow> Create(std::shared_ptr<SGWidgetContainer> p, v2i pos, v2i size);
        static SGConfig window_config;
    };

    class SGLabel : public SGWidget, public SGTextBase {
    public:
        virtual ~SGLabel();
        virtual void PostResize(bool res, bool rep);
        virtual bool AllowFocus() const { return false; }
        virtual std::shared_ptr<SGWidget> GetDragingTarget() { return parent.lock(); }
        virtual void UpdateVertices();
        virtual void Draw();
        virtual void SetFont(sf::Font* ft, unsigned int sz);
        virtual void SetSpacing(unsigned int x, unsigned int y);
        virtual v2i GetTextOffset();
        virtual int GetMaxWidth();
        virtual bool IsMultiLine();
        virtual void ClearText();
        virtual void SetText(const std::wstring& t, unsigned int cl);
        virtual void AppendText(const std::wstring& t, unsigned int cl);
        
    protected:
        unsigned int max_width = 0;
        
    public:
        static std::shared_ptr<SGLabel> Create(std::shared_ptr<SGWidgetContainer> p, v2i pos, const std::wstring& t, int mw = 0xffff);
        static SGConfig label_config;
    };

    class SGIconLabel : public SGLabel {
    public:
        virtual ~SGIconLabel();
        virtual void EvaluateSize(const std::wstring& t = L"");
        virtual void UpdateTextVertex();
        virtual void Draw();
        
    protected:
        int icon_size = 0;
        int icon_mem_size = 0;
        unsigned int vbo[2] = {0, 0};
        
    public:
        static std::shared_ptr<SGIconLabel> Create(std::shared_ptr<SGWidgetContainer> p, v2i pos, const std::wstring& t, int mw = 0xffff);
        static SGConfig iconlabel_config;
    };
    
    class SGSprite : public SGWidget, public SGSpriteBase {
    public:
        virtual ~SGSprite();
        virtual void UpdateVertices();
        virtual void Draw();
        virtual v2i GetImageOffset();
        
    public:
        static std::shared_ptr<SGSprite> Create(std::shared_ptr<SGWidgetContainer> p, v2i pos, v2i size);
        static SGConfig sprite_config;
    };

    class SGButton : public SGWidget, public SGSpriteBase, public SGTextBase {
    public:
        virtual ~SGButton();
        virtual void UpdateVertices();
        virtual void Draw();
        virtual v2i GetTextOffset();
        virtual int GetMaxWidth();
        virtual bool IsMultiLine();
        virtual v2i GetImageOffset();
        void SetTextureRect(sf::IntRect r1, sf::IntRect r2, sf::IntRect r3, int lw, int rw);
        void SetTexture(sf::Texture* tex);
        
        SGEventHandler<SGWidget> eventButtonClick;
        
    protected:
        virtual bool EventMouseEnter();
        virtual bool EventMouseLeave();
        virtual bool EventMouseButtonDown(sf::Event::MouseButtonEvent evt);
        virtual bool EventMouseButtonUp(sf::Event::MouseButtonEvent evt);
        
        unsigned int vbo[2] = {0, 0};
        unsigned int state = 0;
        int lwidth = 0;
        int rwidth = 0;
        sf::Rect<int> tex_rect[3];
        sf::Texture* tex_texture = nullptr;
        
    public:
        static std::shared_ptr<SGButton> Create(std::shared_ptr<SGWidgetContainer> p, v2i pos, v2i size);
        static SGConfig button_config;
    };

    class SGCheckbox : public SGWidget, public SGTextBase {
    public:
        virtual ~SGCheckbox();
        virtual void PostResize(bool res, bool rep);
        virtual void UpdateVertices();
        virtual void Draw();
        virtual void SetFont(sf::Font* ft, unsigned int sz);
        virtual void SetSpacing(unsigned int x, unsigned int y);
        virtual v2i GetTextOffset();
        virtual int GetMaxWidth();
        virtual bool IsMultiLine();
        virtual void ClearText();
        virtual void SetText(const std::wstring& t, unsigned int cl);
        virtual void AppendText(const std::wstring& t, unsigned int cl);
        
        virtual void SetChecked(bool chk);
        virtual bool IsChecked() { return checked; }
        SGEventHandler<SGWidget, bool> eventCheckChange;
        
    protected:
        virtual bool EventMouseEnter();
        virtual bool EventMouseLeave();
        virtual bool EventMouseButtonDown(sf::Event::MouseButtonEvent evt);
        virtual bool EventMouseButtonUp(sf::Event::MouseButtonEvent evt);
        
        unsigned int vbo[2] = {0, 0};
        unsigned int state = 0;
        bool checked = false;
    
    public:
        static std::shared_ptr<SGCheckbox> Create(std::shared_ptr<SGWidgetContainer> p, v2i pos, const std::wstring& t);
        static SGConfig checkbox_config;
    };

    class SGRadio;

    class SGRadioGroup {
    public:
        void AddRadio(SGRadio* rdo) { rdoset.insert(rdo); }
        void RemoveRadio(SGRadio* rdo) {
            rdoset.erase(rdo);
            if(current == rdo)
                current = nullptr;
        }
        void MakeCurrent(SGRadio* rdo) { current = rdo; }
        SGRadio* GetCurrent() { return current; }
        
    protected:
        std::set<SGRadio*> rdoset;
        SGRadio* current = nullptr;
    };

    class SGRadio : public SGCheckbox {
    public:
        virtual ~SGRadio();
        virtual void UpdateVertices();
        virtual void SetChecked(bool chk);
        void SetGroup(std::shared_ptr<SGRadio> rdo);
        std::shared_ptr<SGRadio> GetCheckedTarget();
        
    protected:
        virtual bool EventMouseButtonUp(sf::Event::MouseButtonEvent evt);
        std::shared_ptr<SGRadioGroup> group;
        
    public:
        static std::shared_ptr<SGRadio> Create(std::shared_ptr<SGWidgetContainer> p, v2i pos, const std::wstring& t);
        static SGConfig radio_config;
    };

    class SGScrollBar : public SGWidget {
        friend class SGTextField;
    public:
        virtual ~SGScrollBar();
        virtual void PostResize(bool res, bool rep);
        virtual std::shared_ptr<SGWidget> GetDragingTarget() { return shared_from_this(); }
        virtual void UpdateVertices();
        virtual void Draw();
        void SetRange(float minv, float maxv, float cur);
        void SetValue(float cur);
        void SetSliderLength(int length);
        
        SGEventHandler<SGWidget, float> eventValueChange;
        
    protected:
        virtual bool EventMouseMove(sf::Event::MouseMoveEvent evt);
        virtual bool EventMouseEnter();
        virtual bool EventMouseLeave();
        virtual bool EventMouseWheel(sf::Event::MouseWheelEvent evt);
        virtual bool DragingBegin(v2i evt);
        virtual bool DragingUpdate(v2i evt);
        virtual bool DragingEnd(v2i evt);
        
        float minvalue = 0.0f;
        float maxvalue = 100.0f;
        int current_pos = 0;
        int pos_min = 0;
        int pos_max = 0;
        int slider_diff = 0;
        int slider_length = 0;
        bool slider_hoving = false;
        bool slider_moving = false;
        bool is_horizontal = true;
        unsigned int vbo[2] = {0, 0};
        
    public:
        static std::shared_ptr<SGScrollBar> Create(std::shared_ptr<SGWidgetContainer> p, v2i pos, v2i size, bool is_h);
        static SGConfig scroll_config;
    };

    class SGTextEdit : public SGWidgetContainer, public SGTextBase {
    public:
        ~SGTextEdit();
        virtual void PostResize(bool res, bool rep);
        virtual std::shared_ptr<SGWidget> GetDragingTarget() { return shared_from_this(); }
        virtual void UpdateVertices();
        virtual void Draw();
        virtual v2i GetTextOffset();
        virtual int GetMaxWidth();
        virtual bool IsMultiLine() { return false; };
        virtual unsigned int GetTextColor(int index);
        void DrawSelectRegion();
        void UpdateSelRegion();
        void SetSelRegion(unsigned int start, unsigned int end);
        void CheckCursorPos();
        void CheckDragPos();
        
        SGEventHandler<SGWidget> eventTextEnter;
        
    protected:

        virtual bool EventMouseEnter();
        virtual bool EventMouseLeave();
        virtual bool EventKeyDown(sf::Event::KeyEvent evt);
        virtual bool EventCharEnter(sf::Event::TextEvent evt);
        virtual bool EventGetFocus();
        virtual bool EventLostFocus();
        virtual bool DragingBegin(v2i evt);
        virtual bool DragingUpdate(v2i evt);
        virtual bool DragingEnd(v2i evt);
        
        unsigned int GetHitIndex(v2i pos);
        
        unsigned int def_color = 0xff000000;
        unsigned int sel_color = 0xffffffff;
        unsigned int sel_bcolor = 0xff000000;
        bool hoving = false;
        bool focus = false;
        bool draging = true;
        int text_offset = 0;
        int drag_check = 0;
        sf::IntRect text_area;
        unsigned int vbo[2] = {0, 0};
        sf::Clock frame_clock;
        float cursor_time = 0.0f;
        unsigned int cursor_pos = 0;
        bool sel_change = true;
        unsigned int sel_start = 0;
        unsigned int sel_end = 0;
        
    public:
        static std::shared_ptr<SGTextEdit> Create(std::shared_ptr<SGWidgetContainer> p, v2i pos, v2i size);
        static SGConfig textedit_config;
    };

    class SGListBox : public SGWidgetContainer, public SGTextBase {
    public:
        ~SGListBox();
        virtual void UpdateVertices();
        virtual void Draw();
        virtual v2i GetTextOffset();
        virtual int GetMaxWidth();
        virtual bool IsMultiLine() { return true; };
        virtual void UpdateTextVertex();
        virtual void DrawText();
        void InsertItem(unsigned int index, std::string& item, unsigned int color);
        void AddItem(std::string& item, unsigned int color);
        
        SGEventHandler<SGWidget, int> eventSelChange;
        SGEventHandler<SGWidget, int> eventDoubleClick;
        
    protected:
        unsigned int sel_color = 0xffffffff;
        unsigned int sel_bcolor = 0xff000000;
        unsigned int color1 = 0xffffffff;
        unsigned int color2 = 0xffeeeeee;
        int current_sel = 0;
        int line_spacing = 0;
        int text_offset = 0;
        sf::IntRect text_area;
        unsigned int vbo[2] = {0, 0};
        std::vector<std::pair<std::string, unsigned int>> items;
        
    public:
        static std::shared_ptr<SGListBox> Create(std::shared_ptr<SGWidgetContainer> p, v2i pos, v2i size);
        static SGConfig listbox_config;
    };
    // ===== GUI Components End =====
    
}

#endif
