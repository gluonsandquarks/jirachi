use iced::widget::{ button, column, pick_list, text, center, slider, text_input, row, horizontal_space, vertical_space };
use iced::widget::{ Column, Row };
use iced::{ Element, Theme, Fill, Color };

const MAX_K_VALUES: f32 = 5000.0;

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
    kp: f32,
    kd: f32,
    ki: f32,
}

#[derive(Debug, Clone)]
enum Message {
    ThemeChange(Theme),
    ScanDevices,
    SelectDevice(String),
    ConnectionSuccess,
    ErrorConnecting,
    ResetApplication,
    KpSliderChanged(f32),
    KpInputBoxChanged(String),
    KdSliderChanged(f32),
    KdInputBoxChanged(String),
    KiSliderChanged(f32),
    KiInputBoxChanged(String),
}

impl State {
    fn update(&mut self, message: Message) {
        match message {
            Message::ThemeChange(theme) => { self.theme = theme; },
            Message::ScanDevices => { self.device_list.push("new device".to_string()); },
            Message::SelectDevice(device) => { self.selected_device = Some(device); self.screen = Screen::LoadingScreen; },
            Message::ConnectionSuccess => { self.screen = Screen::ControlScreen; },
            Message::ErrorConnecting => { self.screen = Screen::ErrorScreen; },
            Message::ResetApplication => {
                self.device_list.clear();
                self.device_list.resize(0, "".to_string());
                self.selected_device = None;
                self.screen = Screen::InitScreen;
                self.kp = 0.0;
                self.kd = 0.0;
                self.ki = 0.0;
            },
            Message::KpSliderChanged(new_kp) => { self.kp = new_kp; }
            Message::KpInputBoxChanged(new_kp) => {
                let mut kp_float = new_kp.parse().unwrap_or(0.0);
                if kp_float > MAX_K_VALUES { kp_float = MAX_K_VALUES; }
                if kp_float < 0.0 { kp_float = 0.0; }
                self.kp = kp_float;
            }
            Message::KdSliderChanged(new_kd) => { self.kd = new_kd; }
            Message::KdInputBoxChanged(new_kd) => {
                let mut kd_float = new_kd.parse().unwrap_or(0.0);
                if kd_float > MAX_K_VALUES { kd_float = MAX_K_VALUES; }
                if kd_float < 0.0 { kd_float = 0.0; }
                self.kd = kd_float;
            }
            Message::KiSliderChanged(new_ki) => { self.ki = new_ki; }
            Message::KiInputBoxChanged(new_ki) => {
                let mut ki_float = new_ki.parse().unwrap_or(0.0);
                if ki_float > MAX_K_VALUES { ki_float = MAX_K_VALUES; }
                if ki_float < 0.0 { ki_float = 0.0; }
                self.ki = ki_float;
            }
            _ => {}
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
            .push(row![button("Die").on_press(Message::ErrorConnecting), horizontal_space(), button("Go!").on_press(Message::ConnectionSuccess)]) /* TODO: need to get this message from a failed BLE connection */
            .push(Self::footer(self))
            .push(Self::madeby("github.com/gluonsandquarks"))
    }

    fn control_screen(&self) -> Column<Message> {
        let kp_str = self.kp.to_string();
        let kd_str = self.kd.to_string();
        let ki_str = self.ki.to_string();
        Self::container("Jirachi - PID Controller")
            .push(row![text("Kp = ").size(20), text_input("Input value for Kp", &kp_str).on_input(Message::KpInputBoxChanged).size(20)])
            .push(slider(0.0..=MAX_K_VALUES, self.kp, Message::KpSliderChanged))
            .push(row![text("Kd = ").size(20), text_input("Input value for Kd", &kd_str).on_input(Message::KdInputBoxChanged).size(20)])
            .push(slider(0.0..=MAX_K_VALUES, self.kd, Message::KdSliderChanged))
            .push(row![text("Ki = ").size(20), text_input("Input value for Ki", &ki_str).on_input(Message::KiInputBoxChanged).size(20)])
            .push(slider(0.0..=MAX_K_VALUES, self.ki, Message::KiSliderChanged))
            .push(button("Fetch values from device").width(Fill))
            .push(button("Upload values to device").width(Fill))
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
            kp: 0.0,
            kd: 0.0,
            ki: 0.0,
        }
    }
}

