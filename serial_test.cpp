#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <string>

#include <fstream>   // zum Schreiben in Dateien
#include <ctime>     // für Zeitstempel

// Zeitstempel erzeugen
std::string aktuellerZeitstempel() {
    time_t jetzt = time(nullptr);
    struct tm* zeitInfo = localtime(&jetzt);
    char puffer[16];
    strftime(puffer, sizeof(puffer), "[%H:%M:%S]", zeitInfo);
    return std::string(puffer);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Nutzung: " << argv[0] << " <serieller-port>" << std::endl;
        std::cerr << "Beispiel: /dev/cu.usbmodem11301" << std::endl;
        return 1;
    }

    const char* portName = argv[1];

    int fd = open(portName, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        std::cerr << "Fehler: Port konnte nicht geoeffnet werden: " << portName << std::endl;
        return 1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "Fehler beim Lesen der Port-Attribute" << std::endl;
        close(fd);
        return 1;
    }

    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 10;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "Fehler beim Setzen der Port-Attribute" << std::endl;
        close(fd);
        return 1;
    }

    sleep(2); // Arduino braucht kurz Zeit nach dem Verbindungsaufbau

    tcflush(fd, TCIOFLUSH);

    std::cout << "Verbindung zu " << portName << " hergestellt." << std::endl;
    std::cout << "Geben Sie einen Ausdruck ein (z.B. 34*72) oder 'exit' zum Beenden:" << std::endl;

    // Log-Datei öffnen (im Anhängemodus, damit bei erneutem Start nichts überschrieben wird)
    std::ofstream logDatei("log.txt", std::ios::app);
    if (!logDatei.is_open()) {
        std::cerr << "Warnung: Log-Datei konnte nicht geoeffnet werden." << std::endl;
    } else {
        logDatei << aktuellerZeitstempel() << " --- Neue Sitzung gestartet ---" << std::endl;
    }

    // Hauptschleife: wiederholt Eingabe -> Senden -> Empfangen
    while (true) {
        std::cout << "\n> ";
        std::string eingabe;
        std::getline(std::cin, eingabe);

        if (eingabe == "exit") {
            std::cout << "Programm wird beendet." << std::endl;
            break;
        }

        if (eingabe.empty()) {
            continue; // leere Eingabe ignorieren, erneut fragen
        }

        // Nachricht senden (mit Zeilenumbruch als Terminator)
        tcflush(fd, TCIOFLUSH); // alte Pufferreste vor jedem neuen Versuch leeren
        std::string nachricht = eingabe + "\n";
        write(fd, nachricht.c_str(), nachricht.length());
        std::cout << "Gesendet: " << eingabe << std::endl;

         if (logDatei.is_open()) {
            logDatei << aktuellerZeitstempel() << " GESENDET: " << eingabe << std::endl;
        }

        // Antwort empfangen (zeichenweise bis \n)
        std::string antwort;
        char zeichen;
        bool zeilenendeGefunden = false;

        for (int versuch = 0; versuch < 100 && !zeilenendeGefunden; versuch++) {
            int n = read(fd, &zeichen, 1);
            if (n > 0) {
                if (zeichen == '\n') {
                    zeilenendeGefunden = true;
                } else {
                    antwort += zeichen;
                }
            }
        }

        if (!antwort.empty()) {
            std::cout << "Empfangen: " << antwort << std::endl;

             if (logDatei.is_open()) {
                logDatei << aktuellerZeitstempel() << " EMPFANGEN: " << antwort << std::endl;
            }

        } else {
            std::cout << "Keine Antwort erhalten (Timeout)" << std::endl;
        }
    }

    if (logDatei.is_open()) {
        logDatei << aktuellerZeitstempel() << " --- Sitzung beendet ---" << std::endl;
        logDatei.close();
    }

    close(fd);
    return 0;
}