//DHT 01 D9
#define DHTPIN1 9 // what pin we're connected to
#define DHTTYPE DHT11 // DHT 11
//#define DHTTYPE DHT22 // DHT 22
//DHT dht(DHTPIN1, DHTTYPE);
//=====================================================
const char Wifi_SSID[]="YOUR_WIFI_SSID"; 
const char Wifi_password[]="YOUR_WIFI_PASSWORD";
// const char Wifi_SSID[]="YOUR_WIFI_SSID_2"; 
// const char Wifi_password[]="YOUR_WIFI_PASSWORD_2";
//const char Wifi_SSID[]="YOUR_WIFI_SSID_3"; 
//const char Wifi_password[]="YOUR_WIFI_PASSWORD";

long serial_speed = 9600;
const char CHI_IOT_Srever[]="iot.cht.com.tw";
const char User_ID[]="YOUR_WIFI_SSID";
const char Project_key[]="YOUR_PROJECT_KEY";
const char Project_Service[]="/v1/device/YOUR_DEVICE_ID";
char Action_RAW[]="/rawdata";
char Action_sensor[]="/sensor/senser_01/csv";
