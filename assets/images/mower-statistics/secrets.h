//Your WiFi credentials
const char ssid[] = "The Promised LAN";
const char password[] =  "AtorpsUdde";

//RTK2Go works well and is free
const char casterHost[] = "rtk2go.com"; 
const uint16_t casterPort = 2101;
const char casterUser[] = "johan.holmberg83@outlook.com"; //User must provide their own email address to use RTK2Go
const char casterUserPW[] = "";
const char mountPoint[] = "se_skpl_akerholmsv"; //The mount point you want to get data from

//Emlid Caster also works well and is free
//const char casterHost[] = "caster.emlid.com";
//const uint16_t casterPort = 2101;
//const char casterUser[] = "u99696"; //User name and pw must be obtained through their web portal
//const char casterUserPW[] = "466zez";
//const char mountPoint[] = "MP1979"; //The mount point you want to get data from

//Your Domain name with URL path or IP address with path
String serverName = "https://eu-central-1.aws.data.mongodb-api.com/app/mowerstatistics-biclg/endpoint/fromMowerGps";
String apiKey = "WckCV5jukR9lKMnSLXGbbkQznHSewF1wvYdsBeTRo53YzjUOpglCM6v4Uemr1Ujl";
