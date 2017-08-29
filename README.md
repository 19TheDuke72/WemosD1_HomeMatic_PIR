# Wemos D1 als PIR-Bewegungsmelder  

## Funktionsweise:
Wird eine Bewegung erkannt, wird eine Logikvariable der CCU bzw. in ioBroker für die Dauer der Haltezeit (einstellbar am Poti des HC-SR501 PIR Bewegungsmelders) auf "wahr" gesetzt und anschließend wieder auf "falsch".

Es lassen sich bis zu 4 PIR-Module anschließen! 

_Desweiteren lassen sich die Pins auch als digitale Eingänge nutzen._

Modul
  - 1 an D5
  - 2 an D6
  - 3 an D1
  - 4 an D2

## Folgende Bauteile werden benötigt:
- Wemos D1 Mini
- 1..4 HC-SR501 PIR Bewegungsmelder
- 1 Taster (nicht dauerhaft, nur um bei erster Inbetriebnahme / Änderungen den Konfigurationsmodus zu starten)
- Stromversorgung (z.B. 5V USB-Netzteil)

![Anschlussplan](Images/wire.png)
## Flashen
Wenn alles nach obigem Bild verdrahtet wurde, kann das Image `WemosD1_HomeMatic_PIR.ino.d1_mini.bin` auf den Wemos geflasht werden.
Die neueste vorkompilierte Version ist bei den [Releases](https://github.com/jp112sdl/WemosD1_HomeMatic_PIR/releases/latest) zu finden

#### Vorgehensweise:
1. Voraussetzungen:
  - CH340-Treiber installieren
  - [esptool](https://github.com/igrr/esptool-ck/releases) herunterladen
2. WemosD1 mit einem microUSB-Kabel an den PC anschließen
3. Bezeichnung des neuen COM-Ports im Gerätemanager notieren (z.B. COM5)
4. Flash-Vorgang durchführen: 

  `esptool.exe -vv -cd nodemcu -cb 921600 -cp COM5 -ca 0x00000 -cf WemosD1_HomeMatic_PIR.ino.d1_mini.bin`

## Voraussetzungen: 
HomeMatic:
  - eine Systemvariable vom Typ "Logikwert". Bei der Benennung möglichst auf Umlaute und Leerzeichen verzichten!
  
ioBroker:
  - ein Datenpunkt vom Typ "Logikwert" (manuell im Tab "Objekte" anlegen)

## Konfiguration des Wemos D1
Um den Konfigurationsmodus zu starten, muss der Wemos D1 mit gedrückt gehaltenem Taster gestartet werden.
Die blaue LED blinkt kurz und leuchtet dann dauerhaft. 

Der Konfigurationsmodus ist nun aktiv.

Auf dem Handy oder Notebook sucht man nun nach neuen WLAN Netzen in der Umgebung. 

Es erscheint ein neues WLAN mit dem Namen "WemosD1-xx:xx:xx:xx:xx:xx"

Nachdem man sich mit diesem verbunden hat, öffnet sich automatisch das Konfigurationsportal.

Geschieht dies nicht nach ein paar Sekunden, ist im Browser die Seite http://192.168.4.1 aufzurufen.

**WLAN konfigurieren auswählen**

**SSID**: WLAN aus der Liste auswählen, oder SSID manuell eingeben

**WLAN-Key**: WLAN Passwort

**Backend**: selbsterklärend

**CCU2 / ioBroker IP**: selbsterklärend

**n. Systemvariable / ObjektID (Dx)**: 

HomeMatic:
  - Name der Systemvariable, die in der CCU angelegt wurde
  
ioBroker:
  - ObjektID (vollständiger Pfad! bspw. admin.0.pir, wenn eine Variable namens pir unter admin.0 angelegt wurde)
  
Die Eingabefelder, an denen kein PIR-Modul angeschlossen ist, sind leer zu lassen.
  
  
## PIR Bewegungsmelder einstellen:
Die Haltezeit und sowie die Empfindlichkeit können an den beiden Potentiometern eingestellt werden.
Eine Beschreibung lässt sich schnell bei Google finden, bzw. im [Datenblatt](https://www.mpja.com/download/31227sc.pdf) nachlesen.

