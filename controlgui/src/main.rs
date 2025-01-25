use std::result;

use iced::widget::{ button, column, pick_list, text, center, slider, text_input, row, horizontal_space, vertical_space, toggler };
use iced::widget::{ Column, Row };
use iced::{ Element, Theme, Fill, Color, Task };
use btleplug::api::{ Central, CharPropFlags, Manager as _, Peripheral as _, ScanFilter };
use btleplug::platform::{ Manager, Adapter, Peripheral };
use uuid::Uuid; /* kinda bloated but i'm lazy rn and don't want to implement uuid from scratch :P */

const MAX_K_VALUES:     f32  = 5000.0;
const TOGGLE_CTRL_UUID: Uuid = Uuid::from_u128(0x0000c0c0_0000_1000_8000_00805f9b34fb);
const READ_KP_UUID:     Uuid = Uuid::from_u128(0x0000aaaa_0000_1000_8000_00805f9b34fb);
const WRIT_KP_UUID:     Uuid = Uuid::from_u128(0x0000aaa1_0000_1000_8000_00805f9b34fb);
const READ_KD_UUID:     Uuid = Uuid::from_u128(0x0000bbbb_0000_1000_8000_00805f9b34fb);
const WRIT_KD_UUID:     Uuid = Uuid::from_u128(0x0000bbb1_0000_1000_8000_00805f9b34fb);
const READ_KI_UUID:     Uuid = Uuid::from_u128(0x0000cccc_0000_1000_8000_00805f9b34fb);
const WRIT_KI_UUID:     Uuid = Uuid::from_u128(0x0000ccc1_0000_1000_8000_00805f9b34fb);

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

#[derive(Debug, Clone)]
enum Error {
    BrokeAssError,
    IOError,
    NoPeripheralsError,
    PeripheralNotFoundError,
}

#[derive(Debug, Clone)]
struct Packet {
    kp: f32,
    kd: f32,
    ki: f32,
    is_ctrl_active: bool,
}

struct State {
    title: String,
    screen: Screen,
    theme: Theme,
    device_list: Vec<String>,
    selected_device: Option<String>,
    scan_ok: bool,
    fetch_ok: bool,
    up_ok: bool,
    kp: f32,
    kd: f32,
    ki: f32,
    is_ctrl_active: bool,
    adapter_list: Option<Vec<Adapter>>,
    ble_peripheral: Option<Peripheral>,
    ble_error: Option<String>,
}

#[derive(Debug, Clone)]
enum Message {
    ThemeChange(Theme),
    ScanDevices,
    ScanFinished(Result<(Option<Vec<Adapter>>, Vec<String>), Error>),
    SelectDevice(String),
    ConnectToDevice,
    ConnectionResult(Result<Peripheral, Error>),
    ResetApplication,
    FetchData,
    FetchDataResult(Result<Packet, Error>),
    UploadData,
    UploadDataResult(Result<(), Error>),
    KpSliderChanged(f32),
    KpInputBoxChanged(String),
    KdSliderChanged(f32),
    KdInputBoxChanged(String),
    KiSliderChanged(f32),
    KiInputBoxChanged(String),
    ToggleControl(bool),
}

impl State {
    /* just handle events hehe */
    fn update(&mut self, message: Message) -> Task<Message> {
        match message {
            Message::ThemeChange(theme) => { self.theme = theme; Task::none() },
            Message::ScanDevices => { self.scan_ok = false; Self::scan_devices_task() },
            Message::ScanFinished(result) => {
                self.scan_ok = true;
                match result {
                    Ok((adapter_list, device_list)) => {
                        self.adapter_list = adapter_list;
                        self.device_list = device_list;
                        self.ble_error = None;
                    },
                    Err(error) => {
                        let mut error_msg = String::from("Oopsie we made a fucky wucky OwO!!! here's your error now fix it urself!!!! >w<: ");
                        match error {
                            Error::BrokeAssError => { error_msg.push_str(&format!("[{:?}] - bruh just buy a BLE adapter lmfao", error)); },
                            Error::IOError => { error_msg.push_str(&format!("[{:?}] - Maybe try again?", error)); },
                            Error::NoPeripheralsError => { error_msg.clear(); error_msg.push_str("No peripherals were found! Try again :P"); },
                            _ => {}
                        }
                        self.ble_error = Some(error_msg);
                    },
                }
                Task::none()
            },
            Message::SelectDevice(device) => { self.selected_device = Some(device); Task::none() },
            Message::ConnectToDevice => {
                self.screen = Screen::LoadingScreen;
                Self::connect_device_task(self.adapter_list.as_ref().unwrap(), self.selected_device.clone().unwrap())
            },
            Message::ConnectionResult(result) => {
                match result {
                    Ok(peripheral) => {
                        self.screen = Screen::ControlScreen;
                        self.ble_peripheral = Some(peripheral);
                    },
                    Err(error) => {
                        self.screen = Screen::ErrorScreen;
                        let error_msg = format!("Error ID: [{:?}] - maybe try again?", error);
                        self.ble_error = Some(error_msg);
                    },
                }
                Task::none()
            },
            Message::ResetApplication => {
                self.device_list.clear();
                self.device_list.resize(0, "".to_string());
                self.selected_device = None;
                self.screen = Screen::InitScreen;
                self.kp = 0.0;
                self.kd = 0.0;
                self.ki = 0.0;
                self.ble_peripheral = None;
                self.ble_error = None;
                Task::none()
            },
            Message::KpSliderChanged(new_kp) => { self.kp = new_kp; Task::none() },
            Message::KpInputBoxChanged(new_kp) => {
                let mut kp_float = new_kp.parse().unwrap_or(0.0);
                if kp_float > MAX_K_VALUES { kp_float = MAX_K_VALUES; }
                if kp_float < 0.0 { kp_float = 0.0; }
                self.kp = kp_float;
                Task::none()
            },
            Message::KdSliderChanged(new_kd) => { self.kd = new_kd; Task::none() },
            Message::KdInputBoxChanged(new_kd) => {
                let mut kd_float = new_kd.parse().unwrap_or(0.0);
                if kd_float > MAX_K_VALUES { kd_float = MAX_K_VALUES; }
                if kd_float < 0.0 { kd_float = 0.0; }
                self.kd = kd_float;
                Task::none()
            },
            Message::KiSliderChanged(new_ki) => { self.ki = new_ki; Task::none() },
            Message::KiInputBoxChanged(new_ki) => {
                let mut ki_float = new_ki.parse().unwrap_or(0.0);
                if ki_float > MAX_K_VALUES { ki_float = MAX_K_VALUES; }
                if ki_float < 0.0 { ki_float = 0.0; }
                self.ki = ki_float;
                Task::none()
            },
            Message::ToggleControl(control) => { self.is_ctrl_active = control; Task::none() },
            Message::FetchData => {
                self.fetch_ok = false;
                self.up_ok = false;
                Self::fetch_data_task(self.ble_peripheral.as_ref().unwrap())
            },
            Message::UploadData => {
                self.fetch_ok = false;
                self.up_ok = false;
                Self::upload_data_task()
            },
            Message::FetchDataResult(result) => {
                self.fetch_ok = true;
                self.up_ok = true;
                match result {
                    Ok(packet) => {
                        self.kp = packet.kp;
                        self.kd = packet.kd;
                        self.ki = packet.ki;
                    },
                    Err(error) => {
                    },
                }
                Task::none()
            },
            _ => { Task::none() },
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
        let scan_btn = if self.scan_ok {
            button("Scan nearby BLE devices").on_press(Message::ScanDevices).width(Fill)
        } else {
            button("Scan nearby BLE devices").width(Fill)
        };

        let connect_btn = if self.selected_device != None {
            button("Connect!").on_press(Message::ConnectToDevice)
        } else {
            button("Connect!")
        };

        let mut error_msg = String::new();
        if let Some(ble_error) = self.ble_error.clone() {
            error_msg.push_str(&ble_error);
        } else {
            error_msg.push_str("");
        }

        Self::container("Jirachi - Device Setup")
            .push(vertical_space())
            .push(scan_btn)
            .push(pick_list(self.device_list.clone(), self.selected_device.clone(), Message::SelectDevice).width(Fill).placeholder("Scan to show device list"))
            .push(text(error_msg).size(20))
            .push(vertical_space())
            .push(row![horizontal_space(), connect_btn])
            .push(Self::footer(self))
            .push(Self::madeby("github.com/gluonsandquarks"))
    }
    
    fn loading_screen(&self) -> Column<Message> {
        let device = self.selected_device.clone();
        Self::container("Loading...")
            .push(vertical_space())
            .push(row![text("Connecting to device: ").size(20), text(device.unwrap_or("".to_string())).size(20)])
            .push(vertical_space())
            .push(Self::footer(self))
            .push(Self::madeby("github.com/gluonsandquarks"))
    }

    fn control_screen(&self) -> Column<Message> {
        let kp_str = self.kp.to_string();
        let kd_str = self.kd.to_string();
        let ki_str = self.ki.to_string();
        let fetch_btn = if self.fetch_ok {
            button("Fetch values from device").on_press(Message::FetchData).style(button::primary).width(Fill)
        } else {
            button("Fetch values from device").style(button::primary).width(Fill)
        };
        let up_btn = if self.up_ok {
            button("Upload values to device").on_press(Message::UploadData).style(button::success).width(Fill)
        } else {
            button("Upload values from device").style(button::success).width(Fill)
        };
        Self::container("Jirachi - PID Controller")
            .push(vertical_space())
            .push(toggler(self.is_ctrl_active).label("Control Enable").on_toggle(Message::ToggleControl))
            .push(row![text("Kp = ").size(20), text_input("Input value for Kp", &kp_str).on_input(Message::KpInputBoxChanged).size(20)])
            .push(slider(0.0..=MAX_K_VALUES, self.kp, Message::KpSliderChanged))
            .push(row![text("Kd = ").size(20), text_input("Input value for Kd", &kd_str).on_input(Message::KdInputBoxChanged).size(20)])
            .push(slider(0.0..=MAX_K_VALUES, self.kd, Message::KdSliderChanged))
            .push(row![text("Ki = ").size(20), text_input("Input value for Ki", &ki_str).on_input(Message::KiInputBoxChanged).size(20)])
            .push(slider(0.0..=MAX_K_VALUES, self.ki, Message::KiSliderChanged))
            .push(fetch_btn)
            .push(up_btn)
            .push(vertical_space())
            .push(row![button("Disconnect").on_press(Message::ResetApplication)].push(Self::footer(self)))
            .push(Self::madeby("github.com/gluonsandquarks"))
    }

    fn error_screen(&self) -> Column<Message> {
        let selected_device = self.selected_device.clone();
        Self::container("Something went wrong...")
            .push(vertical_space())
            .push(column![text("Something went wrong when we were trying to connect to your selected device: "), text(selected_device.unwrap())])
            .push(text(self.ble_error.as_ref().unwrap()))
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

    async fn scan_devices() -> Result<(Option<Vec<Adapter>>, Vec<String>), Error> {
        let mut device_list = Vec::<String>::new();
        let manager = Manager::new().await.map_err(|_| Error::IOError)?;
        let adapter_list = manager.adapters().await.map_err(|_| Error::IOError)?;
        if adapter_list.is_empty() {
            return Err(Error::BrokeAssError);
        }
        for adapter in adapter_list.iter() {
            let _ = adapter.start_scan(ScanFilter::default()).await.map_err(|_| Error::IOError);

            /*
             * btleplug example for scanning literally used a sleep to wait until the BLE peripheral returned a successful scan,
             * since i'm not in the tokio runtime context and the futures implementation of iced doesn't have a sleep mechanism 
             * (as far as i know but im lazy cause i didnt want to spend too much looking for a more "legit" solution :p)
             * i just did what they did back in the day lmfao
             * there's probably a more civilized way to do this through the btleplug api but idc i'll just do a fucky wucky >w<
             */
            // time::sleep(Duration::from_secs(2)).await;
            // ^~~~ registered offender
            for _ in 0..99999999u64 {
                /* lmfaoooooo, this probably gets optimized out by the compiler?? TODO: check if this is true */
            }

            let peripherals = adapter.peripherals().await.map_err(|_| Error::IOError)?;

            if peripherals.is_empty() {
                return Err(Error::NoPeripheralsError);
            } else {
                for peripheral in peripherals.iter() {
                    let properties = peripheral.properties().await.map_err(|_| Error::IOError)?;
                    let _ = peripheral.is_connected().await.map_err(|_| Error::IOError)?;
                    device_list.push(properties.unwrap().local_name.unwrap_or(String::from("{ Peripheral name unknown }")))
                }
            }
        }
        return Ok((Some(adapter_list), device_list));
    }

    fn scan_devices_task() -> Task<Message> {
        Task::perform(Self::scan_devices(), Message::ScanFinished)
    }

    async fn connect_device(adapter: Vec<Adapter>, peripheral_name: String) -> Result<Peripheral, Error> {

        let mut target_peripheral: Option<Peripheral> = None;

        for adapter in adapter.iter() {
            let peripherals = adapter.peripherals().await.map_err(|_| Error::IOError)?;
            if peripherals.is_empty() { return Err(Error::NoPeripheralsError); }
            else {
                for peripheral in peripherals.iter() {
                    let properties = peripheral.properties().await.map_err(|_| Error::IOError)?;
                    let is_connected = peripheral.is_connected().await.map_err(|_| Error::IOError)?;
                    /* unless we get extremely unlucky and someone actually advertises their device like "ignore this shit :PP", */
                    /* this will ignore any unknown device, otherwise it will try to connect to it XDDD */
                    let local_name = properties.unwrap().local_name.unwrap_or(String::from("ignore this shit :PP"));

                    /* check if it's the peripheral we want */
                    if local_name.contains(peripheral_name.as_str()) {
                        println!("Found matching peripheral {:?}...", &local_name);
                        if !is_connected {
                            /* connect if we aren't already connected */
                            if let Err(err) = peripheral.connect().await {
                                eprintln!("Error connecting to the peripheral!!! fukkk this is the reason: {}", err);
                                return Err(Error::IOError);
                            }
                        }
                        /* check once again if we connected successfully */
                        let is_connected = peripheral.is_connected().await.map_err(|_| Error::IOError)?;
                        if is_connected {
                            println!("yaaaay we made it, connected to peripheral {}", local_name);
                            target_peripheral = Some(peripheral.clone());
                            break;
                        }
                    }
                }
            }
            if !target_peripheral.is_none() { return Ok(target_peripheral.unwrap()); } /* don't keep searching in the adapters if we already have the target peripheral */
        }

        Err(Error::PeripheralNotFoundError)
    }

    fn connect_device_task(adapter: &Vec<Adapter>, peripheral_name: String) -> Task<Message> {
        let cloned_adapter = adapter.clone();        /* clone before borrowing cause rust :P */
        let cloned_p_name = peripheral_name.clone(); /* see above!!!! */
        Task::perform(Self::connect_device(cloned_adapter, cloned_p_name), Message::ConnectionResult)
    }

    async fn fetch_data(peripheral: Peripheral) -> Result<Packet, Error> {
        let mut packet = Packet{ kp: 0.0, kd: 0.0, ki: 0.0, is_ctrl_active: false, };

        /* get the available services from the peripheral */
        peripheral.discover_services().await.map_err(|_| Error::IOError)?;

        /* this urgently needs refactoring lmao */
        for characteristic in peripheral.characteristics() {
            match characteristic.uuid {
                READ_KP_UUID => {
                    let read_bytes = peripheral.read(&characteristic).await.map_err(|_| Error::IOError)?;
                    let mut result_string = String::new();
                    for byte in read_bytes {
                        result_string.push(byte as char);
                    }
                    packet.kp = result_string.parse().unwrap_or(0.0);
                },
                READ_KD_UUID => {
                    let read_bytes = peripheral.read(&characteristic).await.map_err(|_| Error::IOError)?;
                    let mut result_string = String::new();
                    for byte in read_bytes {
                        result_string.push(byte as char);
                    }
                    packet.kd = result_string.parse().unwrap_or(0.0);
                },
                READ_KI_UUID => {
                    let read_bytes = peripheral.read(&characteristic).await.map_err(|_| Error::IOError)?;
                    let mut result_string = String::new();
                    for byte in read_bytes {
                        result_string.push(byte as char);
                    }
                    packet.ki = result_string.parse().unwrap_or(0.0);
                },
                _ => {},
            }
            
        }

        Ok(packet)
    }

    fn fetch_data_task(peripheral: &Peripheral) -> Task<Message> {
        let cloned_peripheral = peripheral.clone();
        Task::perform(Self::fetch_data(cloned_peripheral), Message::FetchDataResult)
    }

    async fn upload_data() -> Result<(), Error> {
        Ok(())
    }

    fn upload_data_task() -> Task<Message> {
        Task::perform(Self::upload_data(), Message::UploadDataResult)
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
            scan_ok: true,
            fetch_ok: true,
            up_ok: true,
            kp: 0.0,
            kd: 0.0,
            ki: 0.0,
            is_ctrl_active: false,
            adapter_list: None,
            ble_peripheral: None,
            ble_error: None,
        }
    }
}

