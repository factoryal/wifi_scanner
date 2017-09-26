#include <WizFi310.h>
#include <openGLCD.h>


#define MAX_AP_LIST 50
#define MAX_NODE 10

enum SECURITY {
	OPEN = 0,
	WPAWPA2
};

enum BAND {
	BAND24 = 1,
	BAND5
};

class APList;

class APInfo {
private:
	friend class APList;
	char SSID[25] = { 0 };
	uint8_t BSSID[6] = { 0 };
	uint8_t RSSI=0;
	uint8_t BandWidth = 0;

	uint8_t Security=0;

	uint8_t RadioBand = BAND24;
	uint8_t Channel=0;

	uint8_t cpy_str2mac(char* strmac) {
		for (uint8_t i = 0; i < 6; i++) {
			uint8_t val = 0;
			uint8_t j = i * 3;
			if (strmac[j] <= '9') val = strmac[j] - '0';
			else val = strmac[j] - 'A' + 10;
			val <<= 4;
			j++;
			if (strmac[j] <= '9') val |= strmac[j] - '0';
			else val |= strmac[j] - 'A' + 10;
			BSSID[i] = val;
		}
		return 1;
	}

	uint8_t cpy_str2security(char* strsec) {
		if (!strcmp(strsec, "Open")) Security = OPEN;
		else if (!strcmp(strsec, "WPAWPA2")) Security = WPAWPA2;
		else return 0;
		return 1;
	}

public:

	APInfo(char* _SSID, char* _BSSID, uint8_t _RSSI, uint8_t _BandWidth, char* _Security, uint8_t _RadioBand, uint8_t _Channel) {
		strncpy(SSID, _SSID, 24);
		cpy_str2mac(_BSSID);
		RSSI = _RSSI;
		BandWidth = _BandWidth;
		cpy_str2security(_Security);
		RadioBand = _RadioBand;
		Channel = _Channel;
	}
	char* getSSID() { return SSID; }
	uint8_t* getBSSID() { return BSSID; }
	uint8_t getRSSI() { return RSSI; }
	uint8_t getBandWidth() { return BandWidth; }
	uint8_t getSecurity() { return Security; }
	uint8_t getRadioBand() { return RadioBand; }
	uint8_t getChannel() { return Channel; }
};

class APList {
private:
	APInfo* list[MAX_AP_LIST];
	uint8_t nList = 0;

	uint8_t swapList(uint8_t a, uint8_t b) { APInfo* tmp = list[a]; list[a] = list[b]; list[b] = tmp; }

public:
	void clear() {
		for (uint8_t i = 0; i < nList; i ++ ) {
			delete list[i];
		}
		memset(list, 0, sizeof(list)*MAX_AP_LIST);
		nList = 0;
	}
	uint8_t add(char* _SSID, char* _BSSID, uint8_t _RSSI, uint8_t _BandWidth, char* _Security, uint8_t _RadioBand, uint8_t _Channel) {
		list[nList] = new APInfo(_SSID, _BSSID, _RSSI, _BandWidth, _Security, _RadioBand, _Channel);
		return nList++;
	}
	
	void printList(uint8_t n) {
		for (uint8_t i = 0; i < 6; Serial.print(':'), i++) { uint8_t a = list[n]->BSSID[i]; if (a < 0x10) Serial.print('0'); Serial.print(a, HEX); } Serial.print('\t');
		Serial.print(getRSSI(n)); Serial.print('\t');
		Serial.print(getBandWidth(n)); Serial.print('\t');
		Serial.print(getSecurity(n)); Serial.print('\t');
		Serial.print(getRadioBand(n)); Serial.print('\t');
		Serial.print(getChannel(n)); Serial.print('\t');
		Serial.print(getSSID(n));
		Serial.print('\n');
	}

	void printChannel() {
		uint8_t spectrum[13] = { 0 };
		Serial.println();
		for (uint8_t i = 0; i < getnList(); i++) spectrum[getChannel(i) - 1]++;
		for (uint8_t i = 0; i < sizeof(spectrum); i++) {
			Serial.print(i + 1); Serial.print("\t: ");
			for (uint8_t j = spectrum[i]; j > 0; j--) Serial.print(']');
			Serial.println();
		}
	}

	void sortRSSI() {
		Serial.print(F("\r\nSorting by RSSI..."));
		//Selection Sort
		uint8_t idx = 0;
		for (uint8_t i = 0; i < nList; i++) {
			uint8_t min = 0xFF;
			uint8_t minidx = i;
			for (uint8_t j = i; j < nList; j++) {
				if (min > list[j]->RSSI) { minidx = j; min = list[j]->RSSI; }
			}
			swapList(minidx, i);
		}
		Serial.print(F("OK"));
	}


	inline uint8_t getnList() { return nList; }
	inline char* getSSID(uint8_t n) { return list[n]->SSID; }
	inline uint8_t* getBSSID(uint8_t n) { return list[n]->BSSID; }
	inline uint8_t getRSSI(uint8_t n) { return list[n]->RSSI; }
	inline uint8_t getBandWidth(uint8_t n) { return list[n]->BandWidth; }
	inline uint8_t getSecurity(uint8_t n) { return list[n]->Security; }
	inline uint8_t getRadioBand(uint8_t n) { return list[n]->RadioBand; }
	inline uint8_t getChannel(uint8_t n) { return list[n]->Channel; }
} APList;

uint8_t scanAP() {
	Serial3.print(F("AT+WSCAN\r"));
	Serial.print(F("\r\nStart scanning..."));
	while (!Serial3.available());
	Serial.print(F("OK"));
	Serial.print(F("\r\nParsing AP data from stream..."));
	Serial3.readStringUntil('/');
	while (Serial3.read() != '\n');
	APList.clear();
	char c;
	uint8_t n = 0;
	while (1) {
		
		//¹öÆÛ ½×±â
		while (Serial3.available() < 5);
		if (Serial3.read() == '[') break;
		else {
			//ÀÎµ¦½º °Ç³Ê¶Ù±â
			while (Serial3.read() != '/');
			//SSID ÆÄ½Ì
			char ssid[25] = { 0 };
			uint8_t idx = 0;
			while (1) {
				while (Serial3.available()<7);
				if ((c = Serial3.read()) == '/') break;
				ssid[idx++] = c;
			}
			//BSSID ÆÄ½Ì
			char bssid[18] = { 0 };
			idx = 0;
			while (1) {
				while (Serial3.available()<17);
				if ((c = Serial3.read()) == '/') break;
				bssid[idx++] = c;
			}
			//RSSI ÆÄ½Ì
			uint8_t rssi;
			char rbuffer[4] = { 0 };
			idx = 0;
			while (1) {
				while (Serial3.available()<3);
				if ((c = Serial3.read()) == '/') break;
				rbuffer[idx++] = c;
			}
			rssi = abs(atoi(rbuffer));
			//bitrate ÆÄ½Ì
			uint8_t bitrate = 0;
			char bbuffer[4] = { 0 };
			idx = 0;
			while (1) {
				while (!Serial3.available());
				if ((c = Serial3.read()) == '/') break;
				bbuffer[idx++] = c;
			}
			bitrate = abs(atoi(bbuffer));
			//security ÆÄ½Ì
			char security[10] = { 0 };
			idx = 0;
			while (1) {
				while (Serial3.available() < 10);
				if ((c = Serial3.read()) == '/') break;
				security[idx++] = c;
			}

			//band ÆÄ½Ì
			c = Serial3.read();
			uint8_t band = 0;
			if (c == '2') band = BAND24;
			else if (c == '5') band = BAND5;
			while (Serial3.read() != '/');

			//channel ÆÄ½Ì
			uint8_t channel;
			char cbuffer[3] = { 0 };
			idx = 0;
			while (1) {
				while (Serial3.available()<3);
				if ((c = Serial3.read()) == '\r') break;
				cbuffer[idx++] = c;
			}
			channel = abs(atoi(cbuffer));
			APList.add(ssid, bssid, rssi, bitrate, security, band, channel);
			n++; 
			
			Serial3.read(); // ¸¶Áö¸· \n Á¦°Å
		}
	}

	Serial.println(F("OK"));
	return 0;
}

//class Node {
//private:
//	uint8_t id;
//	char desc[10];
//	uint8_t nAfter = 0;
//	uint8_t before = NULL, *after = NULL;
//public:
//	Node(uint8_t _id, const char _desc[] = "blank") : id(_id) { strncpy(desc, _desc, 9); }
//	uint8_t getId() { return id; }
//	uint8_t getBefore() { return before; }
//	uint8_t* getAfter() { return after; }
//	void setBefore(uint8_t n) { before = n; }
//	void setAfter(uint8_t n) { after[nAfter++] = n; }
//};
//
///*///////////////////////////////////////////////////////
//±â´É ¼±ÅÃ	-	AP Å½»ö		-	AP Á¤º¸		-	AP ÃßÀû
//			-	½ºÆåÆ®·³					-	AP ¿¬°á
//			-	ÇÖ½ºÆÌ		-	º¸¾È,Ã¤³Î	-	¿¬°á Á¤º¸
//*////////////////////////////////////////////////////////
class MenuTree {
private:
	Node* node[MAX_NODE];
	Node* currentNode = NULL;

	// ·Îµù ¸Þ´º

public:
	

	void createMenuTree() {

	}
} MenuTree;

void loadMenu() {
}

class Node {
private:
	Node* up = NULL;
	Node* down = NULL;
	Node* left = NULL;
	Node* right = NULL;
	enum M_TYPE {
		INIT=1,
		TOP,
		APSEARCH,
		APLISTINFO,
		APTRACKING,
		SPECTRUM
	};
	uint8_t type = 0;
	
public:
	Node(uint8_t t) : type(t) {};
	void setLeft(Node* left) { this->left = left; }
	void setLeft(Node* right) { this->right = right; }
	void setLeft(Node* up) { this->up = up; }
	void setLeft(Node* down) { this->down = down; }
};


void glcd_print_ap(void) {
	GLCD.ClearScreen();
	GLCD.CursorToXY(0, 0);
	GLCD.print("SSID");
	GLCD.CursorToXY(115, 0);
	GLCD.print("Auth");
	GLCD.CursorToXY(140, 0);
	GLCD.print("Ch.");
	GLCD.CursorToXY(165, 0);
	GLCD.print("RSSI");
	for (int i = 0; i < 7; i++) {
		GLCD.CursorToXY(0, (SYSTEM5x7_HEIGHT + 1)*(i + 1));
		GLCD.print(APList.getSSID(i));
		GLCD.CursorToXY(125, (SYSTEM5x7_HEIGHT + 1)*(i + 1));
		GLCD.print(APList.getSecurity(i));
		GLCD.CursorToXY(147, (SYSTEM5x7_HEIGHT + 1)*(i + 1));
		GLCD.print(APList.getChannel(i));
		GLCD.CursorToXY(165, (SYSTEM5x7_HEIGHT + 1)*(i + 1));
		GLCD.print(-APList.getRSSI(i));
	}
}


void setup() {
	GLCD.Init();
	GLCD.SelectFont(System5x7);
	GLCD.print("Init UART...");
	Serial.begin(115200);
	Serial.print(F("\r\n********\r\nSerial ready."));
	Serial.print(F("\r\nInitiallizing Wi-Fi module..."));
	Serial3.begin(115200);
	GLCD.println("OK");
	GLCD.print("Init Wi-Fi...");
	WiFi.init(&Serial3);
	GLCD.println("OK");
	Serial.print(F("OK"));

	MenuTree.createMenuTree();
	GLCD.ClearScreen();
	APList.clear();
}

void loop() {
	//if (Serial.available()) {
		Serial.read();
		scanAP();
		APList.sortRSSI();

		//Serial.print(F("\r\nBSSID\t\t\tRSSI\tspeed\tauth\tband\tchannel\tSSID\n"));
		//for (uint8_t i = 0; i < APList.getnList(); i++) APList.printList(i);
		//APList.printChannel();
		glcd_print_ap();
		
	//}
}

