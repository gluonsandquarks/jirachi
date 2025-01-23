use iced::widget::{ button, column, pick_list, text, center, slider, container, row, horizontal_space, vertical_space };
use iced::widget::{ Column, Row };
use iced::{ Element, Theme, Fill, Color };

pub fn main() -> iced::Result {
    iced::application(State::title, State::update, State::view)
        .theme(State::theme)
        .run()
}

enum Screen {
    InitScreen,
    LoadingScreen,
    ControlScreen,
    ErrorScreen,
}

struct State {
    title: String,
    screen: Screen,
    theme: Theme,
    device_list: Vec<String>,
    selected_device: Option<String>,
}

#[derive(Debug, Clone)]
enum Message {
    ThemeChange(Theme),
    ScanDevices,
    SelectDevice(String),
    ErrorConnecting,
    ResetApplication,
}

impl State {
    fn update(&mut self, message: Message) {
        match message {
            Message::ThemeChange(theme) => { self.theme = theme; },
            Message::ScanDevices => { self.device_list.push("new device".to_string()); },
            Message::SelectDevice(device) => { self.selected_device = Some(device); self.screen = Screen::LoadingScreen; },
            Message::ErrorConnecting => { self.screen = Screen::ErrorScreen; }
            Message::ResetApplication => {
                self.device_list.clear();
                self.device_list.resize(0, "".to_string());
                self.selected_device = None;
                self.screen = Screen::InitScreen;
            }
            _ => { }
        }
    }

    fn view(&self) -> Element<Message> {
        let screen = match self.screen {
            Screen::InitScreen => self.init_screen(),
            Screen::LoadingScreen => self.loading_screen(),
            Screen::ControlScreen => self.control_screen(),
            Screen::ErrorScreen => self.error_screen(),
        };

        let content = column![
            screen,
        ]
        .spacing(50)
        .padding(20)
        .max_width(600);

        center(content).into()
    }

    fn theme(&self) -> Theme {
        self.theme.clone()
    }

    fn title(&self) -> String {
        self.title.clone()
    }

    fn init_screen(&self) -> Column<Message> {
        let container = Self::container("Jirachi - Device Setup");
            container.push(button("Scan nearby BLE devices").on_press(Message::ScanDevices).width(Fill))
            .push(pick_list(self.device_list.clone(), self.selected_device.clone(),Message::SelectDevice).width(Fill).placeholder("Scan to show device list"))
            .push(vertical_space())
            .push(Self::footer(self))
            .push(Self::madeby("github.com/gluonsandquarks"))
    }
    
    fn loading_screen(&self) -> Column<Message> {
        let device = self.selected_device.clone();
        Self::container("Loading...")
            .push(vertical_space())
            .push("Connecting to device: ")
            .push(text(device.unwrap_or("".to_string())))
            .push(vertical_space())
            .push(button("die").on_press(Message::ErrorConnecting)) /* TODO: need to get this message from a failed BLE connection */
            .push(Self::footer(self))
            .push(Self::madeby("github.com/gluonsandquarks"))
    }

    fn control_screen(&self) -> Column<Message> {
        Self::container("Jirachi - PID Controller")
            .push(vertical_space())
            .push(row![button("Reset").on_press(Message::ResetApplication)].push(Self::footer(self)))
            .push(Self::madeby("github.com/gluonsandquarks"))
    }

    fn error_screen(&self) -> Column<Message> {
        let selected_device = self.selected_device.clone();
        Self::container("Something went wrong...")
            .push("Something went wrong when we were trying to connect to your selected device ")
            .push(text(selected_device.unwrap()))
            .push(vertical_space())
            .push(row![button("Reset").on_press(Message::ResetApplication)].push(Self::footer(self)))
            .push(Self::madeby("github.com/gluonsandquarks"))
    }

    fn container(title: &str) -> Column<'_, Message> {
        column![text(title).size(30)].spacing(10)
    }

    fn footer(&self) -> Row<Message> {
        row![
            horizontal_space(),
            text("Theme: ").size(18),
            pick_list(vec![Theme::Light, Theme::Dark], Some(&self.theme), Message::ThemeChange),
        ]
    }
    
    fn madeby(slice: &str) -> Row<Message> {
        row![
            horizontal_space(),
            text(slice).color(Color{r: 0.5, g: 0.5, b: 0.5, a: 0.8}),
        ]
    }

}

/* implement default state to initialize state struct */
impl Default for State {
    fn default() -> Self {
        Self {
            title: "Jirachi PID Controller".to_string(),
            screen: Screen::InitScreen,
            theme: Theme::Dark,
            device_list: Vec::<String>::new(),
            selected_device: None,
        }
    }
}

