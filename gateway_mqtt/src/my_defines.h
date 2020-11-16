// Set this node's subscribe and publish topic prefix
#define MY_MQTT_PUBLISH_TOPIC_PREFIX "myaddress/mysgw-out/433"
#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "myaddress/mysgw-in/433"

// Set MQTT client id
#define MY_MQTT_CLIENT_ID "mys_myaddress"

// Enable these if your MQTT broker requires username/password
//#define MY_MQTT_USER "username"
//#define MY_MQTT_PASSWORD "password"

// Set WIFI SSID and password
#define MY_WIFI_SSID "MY_SSID"
#define MY_WIFI_PASSWORD "theverysecretpasword"

// Set the hostname for the WiFi Client. This is the hostname
// passed to the DHCP server if not static.
#define MY_HOSTNAME "Wemos_MYS_MQTT_GW"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
//#define MY_IP_ADDRESS 192,168,178,87

// If using static ip you can define Gateway and Subnet address as well
//#define MY_IP_GATEWAY_ADDRESS 192,168,178,1
//#define MY_IP_SUBNET_ADDRESS 255,255,255,0

// MQTT broker ip address.
//#define MY_CONTROLLER_IP_ADDRESS 192, 168, 178, 68

//MQTT broker if using URL instead of ip address.
#define MY_CONTROLLER_URL_ADDRESS "my.domain.com"

// The MQTT broker port to to open
#define MY_PORT 1883