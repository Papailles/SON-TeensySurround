import sys
from PyQt5.QtWidgets import (QApplication, QMainWindow, QVBoxLayout, QHBoxLayout,
                             QWidget, QSlider, QPushButton, QLabel, QProgressBar)
from PyQt5.QtCore import Qt, QTimer, QPointF
from PyQt5.QtGui import QPainter, QPen, QBrush
import serial
import time
import math

class AngleWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.angle = 0  # Angle en degrés

    def setAngle(self, angle):
        self.angle = angle
        self.update()  # Demande le redessin du widget

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        # Centre et rayon du cercle
        w, h = self.width(), self.height()
        cx, cy = w / 2, h / 2
        radius = min(cx, cy) - 10

        # Dessiner le cercle
        pen = QPen(Qt.black, 2)
        painter.setPen(pen)
        painter.drawEllipse(QPointF(cx, cy), radius, radius)

        # Calcul de la position du point mobile (0° en haut, inversion de la direction)
        rad = math.radians(-self.angle)
        x = cx + radius * math.cos(rad - math.pi/2)
        y = cy + radius * math.sin(rad - math.pi/2)

        # Dessiner le point mobile (petit cercle rouge)
        brush = QBrush(Qt.red)
        painter.setBrush(brush)
        pointSize = 10
        painter.drawEllipse(QPointF(x, y), pointSize/2, pointSize/2)

        # Afficher l'angle courant au centre du cercle
        painter.setPen(Qt.black)
        font = painter.font()
        font.setPointSize(12)
        painter.setFont(font)
        text = "{}°".format(self.angle)
        painter.drawText(self.rect(), Qt.AlignCenter, text)

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Spatial HRTF Control")
        self.interfaceConnected = False
        self.autoMode = True  # Par défaut, mode auto
        self.musicPaused = False

        try:
            self.serial_port = serial.Serial('COM3', 115200, timeout=0.1)
        except Exception as e:
            print("Erreur lors de l'ouverture du port série:", e)
            sys.exit(1)

        self.initUI()

        # Timer pour lire les messages entrants (toutes les 100 ms)
        self.read_timer = QTimer(self)
        self.read_timer.timeout.connect(self.processIncomingMessages)
        self.read_timer.start(100)

        # Timer pour envoyer GET_ANGLE en mode auto (toutes les 500 ms)
        self.get_angle_timer = QTimer(self)
        self.get_angle_timer.timeout.connect(self.sendGetAngle)
        self.get_angle_timer.start(500)

    def initUI(self):
        # Création d'une mise en page horizontale pour deux colonnes
        main_layout = QHBoxLayout()

        # Colonne de gauche : Contrôles audio existants
        left_layout = QVBoxLayout()
        self.connect_button = QPushButton("Connect", self)
        self.connect_button.clicked.connect(self.connectDevice)
        left_layout.addWidget(self.connect_button)

        self.mode_button = QPushButton("Mode Auto", self)
        self.mode_button.setCheckable(True)
        self.mode_button.clicked.connect(self.modeChanged)
        left_layout.addWidget(self.mode_button)

        self.angle_widget = AngleWidget(self)
        self.angle_widget.setFixedSize(200, 200)
        left_layout.addWidget(self.angle_widget)

        self.selected_angle_label = QLabel("Selected Angle: 0°", self)
        left_layout.addWidget(self.selected_angle_label)

        self.slider = QSlider(Qt.Horizontal, self)
        self.slider.setMinimum(0)
        self.slider.setMaximum(359)
        self.slider.setValue(0)
        self.slider.valueChanged.connect(self.angleChanged)
        left_layout.addWidget(self.slider)

        self.current_track_label = QLabel("Track: None", self)
        left_layout.addWidget(self.current_track_label)

        self.progress_bar = QProgressBar(self)
        self.progress_bar.setRange(0, 100)
        self.progress_bar.setValue(0)
        left_layout.addWidget(self.progress_bar)

        music_layout = QHBoxLayout()
        self.prev_button = QPushButton("Previous", self)
        self.prev_button.clicked.connect(self.prevTrack)
        music_layout.addWidget(self.prev_button)

        self.pause_button = QPushButton("Pause", self)
        self.pause_button.setCheckable(True)
        self.pause_button.clicked.connect(self.pauseTrack)
        music_layout.addWidget(self.pause_button)

        self.next_button = QPushButton("Next", self)
        self.next_button.clicked.connect(self.nextTrack)
        music_layout.addWidget(self.next_button)
        left_layout.addLayout(music_layout)

        main_layout.addLayout(left_layout)

        # Colonne de droite : Contrôle du volume
        right_layout = QVBoxLayout()
        volume_label = QLabel("Volume", self)
        volume_label.setAlignment(Qt.AlignCenter)
        right_layout.addWidget(volume_label)

        self.volume_slider = QSlider(Qt.Vertical, self)
        self.volume_slider.setMinimum(0)
        self.volume_slider.setMaximum(100)
        self.volume_slider.setValue(40)  # Valeur initiale (40%)
        self.volume_slider.valueChanged.connect(self.volumeChanged)
        right_layout.addWidget(self.volume_slider)

        self.volume_value_label = QLabel("40%", self)
        self.volume_value_label.setAlignment(Qt.AlignCenter)
        right_layout.addWidget(self.volume_value_label)

        main_layout.addLayout(right_layout)

        container = QWidget()
        container.setLayout(main_layout)
        self.setCentralWidget(container)

    def connectDevice(self):
        self.sendCommand("CONNECT")
        self.connect_button.setEnabled(False)
        self.interfaceConnected = True

    def modeChanged(self):
        if self.mode_button.isChecked():
            self.mode_button.setText("Mode Manuel")
            self.sendCommand("MODE:MANUEL")
            self.autoMode = False
        else:
            self.mode_button.setText("Mode Auto")
            self.sendCommand("MODE:AUTO")
            self.autoMode = True

    def angleChanged(self, value):
        self.selected_angle_label.setText("Selected Angle: {}°".format(value))
        self.sendCommand("SET_ANGLE:{}".format(value))
        self.sendCommand("GET_ANGLE")  # Actualise immédiatement l'affichage du cercle

    def volumeChanged(self, value):
        # Met à jour le label de volume et envoie la commande au Teensy
        self.volume_value_label.setText("{}%".format(value))
        self.sendCommand("VOLUME:{}".format(value))

    def sendGetAngle(self):
        if self.interfaceConnected and self.autoMode:
            self.sendCommand("GET_ANGLE")

    def processIncomingMessages(self):
        if not self.interfaceConnected or not self.serial_port.isOpen():
            return
        while self.serial_port.in_waiting:
            try:
                line = self.serial_port.readline().decode('utf-8').strip()
            except Exception as e:
                print("Erreur lors du décodage du message:", e)
                continue
            if line.startswith("GET_ANGLE:"):
                angle_value = line.split(":", 1)[1]
                try:
                    angle_int = int(angle_value)
                    self.angle_widget.setAngle(angle_int)
                except ValueError:
                    print("Valeur d'angle invalide:", angle_value)
            elif line.startswith("SET_ANGLE:"):
                angle_value = line.split(":", 1)[1]
                self.selected_angle_label.setText("Selected Angle: {}°".format(angle_value))
            elif line.startswith("TRACK:"):
                track_title = line.split(":", 1)[1]
                self.current_track_label.setText("Track: {}".format(track_title))
            elif line.startswith("PROGRESS:"):
                progress_value = line.split(":", 1)[1]
                try:
                    progress_int = int(progress_value)
                    self.progress_bar.setValue(progress_int)
                except ValueError:
                    print("Valeur de progression invalide:", progress_value)
            elif line.startswith("VOLUME:"):
                # Optionnel : Si le Teensy renvoie une confirmation de volume
                volume_value = line.split(":", 1)[1]
                self.volume_value_label.setText("{}%".format(volume_value))
            else:
                print("Message inconnu:", line)

    def sendCommand(self, cmd):
        if self.serial_port.isOpen():
            command = cmd + "\n"
            self.serial_port.write(command.encode('utf-8'))
            time.sleep(0.05)

    def prevTrack(self):
        self.sendCommand("PREV")

    def nextTrack(self):
        self.sendCommand("NEXT")

    def pauseTrack(self):
        if self.pause_button.isChecked():
            self.pause_button.setText("Play")
            self.sendCommand("PAUSE")
        else:
            self.pause_button.setText("Pause")
            self.sendCommand("PLAY")

    def closeEvent(self, event):
        self.serial_port.close()
        event.accept()

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
