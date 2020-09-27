# v1实现Window Events回调功能

参考资料
[深入理解回调函数](https://flat2010.github.io/2017/01/10/%E6%B7%B1%E5%85%A5%E7%90%86%E8%A7%A3%E5%9B%9E%E8%B0%83%E5%87%BD%E6%95%B0/#2-2-7-GDB%E8%B7%9F%E8%B8%AA)

- 事件类

```c++
//事件类型
enum class EventType
{
    None = 0,
    WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
    AppTick, AppUpdate, AppRender,
    KeyPressed, KeyReleased, KeyTyped,
    MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
};
//事件种类
enum EventCategory
{
    None = 0,
    EventCategoryApplication = BIT(0),
    EventCategoryInput = BIT(1),
    EventCategoryKeyboard = BIT(2),
    EventCategoryMouse = BIT(3),
    EventCategoryMouseButton = BIT(4)
};
//抽象事件类
class Event
{
public:
    virtual ~Event() = default;

    bool Handled = false;

    virtual EventType GetEventType() const = 0;
    virtual const char* GetName() const = 0;
    virtual int GetCategoryFlags() const = 0;
    virtual std::string ToString() const { return GetName(); }

    bool IsInCategory(EventCategory category)
    {
        return GetCategoryFlags() & category;
    }
};

//具体事件实现
class WindowResizeEvent : public Event
{
public:
    WindowResizeEvent(unsigned int width, unsigned int height)
        : m_Width(width), m_Height(height) {}

    unsigned int GetWidth() const { return m_Width; }
    unsigned int GetHeight() const { return m_Height; }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
        return ss.str();
    }

    EVENT_CLASS_TYPE(WindowResize)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
private:
    unsigned int m_Width, m_Height;
};

//事件转发，Dispatch利用OnXXXEvent函数作为参数，真正处理事件
class EventDispatcher
{
    template<typename T> using EventFn = std::function<bool(T&)>;
public:
    EventDispatcher(Event& event)
        : m_Event(event)
    {}

    // F will be deduced by the compiler
    template<typename T>
    bool Dispatch(const EventFn<T>& func)
    {
        if (m_Event.GetEventType() == T::GetStaticType())
        {
            m_Event.Handled = func(static_cast<T&>(m_Event));
            return true;
        }
        return false;
    }
    
private:
    Event& m_Event;
};

//重载运算符，输出事件字符流
inline std::ostream& operator<<(std::ostream& os, const Event& e)
{
    return os << e.ToString();
}
```

具体事件分别在各自类中实现

- 纯虚类 Window

```c++
//主要功能

// 命名参数为Event&的函数
using EventCallbackFn = std::function<void(Event&)>;
//窗口更新
virtual void OnUpdate() = 0;
// 设置事件回调函数
virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
//静态窗口创建，由具体平台实现
static Window* Create(const WindowProps& props = WindowProps());
```

- 平台依赖 WindowsWindow

```c++
// h文件
void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback;  }

GLFWwindow* m_Window;
//实际保存窗口数据的结构体
struct WindowData
{
    std::string Title;
    unsigned int Width, Height;
    bool VSync;

    EventCallbackFn EventCallback;
};

WindowData m_Data;

// cpp文件
//实现Create，构造函数中利用opengl进行创建window的工作，以及使用opengl回调函数
Window* Window::Create(const WindowProps& props)
{
    return new WindowsWindow(props);
}
//构造函数中实际的工作由init来做
void WindowsWindow::Init(const WindowProps& props)
{	
    //创建窗口
    m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(m_Window);
    glfwSetWindowUserPointer(m_Window, &m_Data);

    //Set GLFW callbacks
    glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
    {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
        data.Width = width;
        data.Height = height;

        WindowResizeEvent event(width, height);
        data.EventCallback(event);
            
    });
}


- Application使用的总体流程


```c++
class Application
{
public:
    //创建窗口，回调函数发生时注册事件
	Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
	}
    //运行
    void Run();
    //事件调用，目前只是窗口关闭的事件
    void OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        //OnWindowClose函数：得到WindowCloseEvent事件后，glfw执行关闭窗口动作
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
    }
private:
    bool OnWindowClose(WindowCloseEvent& e);

    std::unique_ptr<Window> m_Window; 
    bool m_Running = true;
};

// To be defined in client
Application* CreateApplication();
```

分析运行如下

```c++
//进入点
app = CreateApplication()
    //客户端
    class Sandbox : public Application  //继承初始化
        //Application
        m_Window = std::unique_ptr<Window>(Window::Create());  //new WindowsWindow, 构造函数中使用glfw回调
        m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
            //窗口resize事件发生时
            系统底层 -> glfwSetWindowSizeCallback -> data.EventCallback(event) 
            //窗口close事件发生时
            系统底层 -> glfwSetWindowCloseCallback -> data.EventCallback(event) -> OnEvent -> dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose)) -> OnWindowClose

app->Run()
    while (m_Running) {m_Window->OnUpdate()}
```

