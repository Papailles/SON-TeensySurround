import sys
from PyQt5.QtWidgets import (QApplication, QMainWindow, QVBoxLayout, QHBoxLayout,
                             QWidget, QSlider, QPushButton, QLabel, QProgressBar,
                             QListWidget, QListWidgetItem)
from PyQt5.QtCore import Qt, QTimer, QPointF
from PyQt5.QtGui import QPainter, QPen, QBrush
import serial
import time
import math

class AngleWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.angle = 0  # Angle en degrés
        self.setMinimumSize(150, 150)  # Taille minimale pour assurer une bonne lisibilité

    def setAngle(self, angle):
        self.angle = angle
        self.update()  # Redessine le widget

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        rect = self.rect()
        cx = rect.center().x()
        cy = rect.center().y()
        # Le rayon est calculé en fonction de la taille du widget, avec une marge de 10 pixels
        radius = min(rect.width(), rect.height()) / 2 - 10

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
        pointSize = 10  # Taille fixe du point
        painter.drawEllipse(QPointF(x, y), pointSize/2, pointSize/2)

        # Afficher l'angle courant au centre du cercle
        painter.setPen(Qt.black)
        font = painter.font()
        font.setPointSize(12)
        painter.setFont(font)
        text = "{}°".format(self.angle)
        painter.drawText(rect, Qt.AlignCenter, text)


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Teensy Surround")
        self.interfaceConnected = False
        self.autoMode = True
        self.musicPaused = False
        self.file_list_entries = []  # Stocke les chaînes "index|filename"

        try:
            self.serial_port = serial.Serial('COM3', 115200, timeout=0.1)
        except Exception as e:
            print("Erreur lors de l'ouverture du port série:", e)
            sys.exit(1)

        self.initUI()

        self.read_timer = QTimer(self)
        self.read_timer.timeout.connect(self.processIncomingMessages)
        self.read_timer.start(100)

        self.get_angle_timer = QTimer(self)
        self.get_angle_timer.timeout.connect(self.sendGetAngle)
        self.get_angle_timer.start(500)

        # Demande automatique de la liste des fichiers 1 seconde après connexion
        QTimer.singleShot(1000, self.requestFileList)

    def initUI(self):
        main_layout = QHBoxLayout()

        # Colonne de gauche : Liste des fichiers audio
        left_layout = QVBoxLayout()
        left_label = QLabel("Fichiers Audio", self)
        left_label.setAlignment(Qt.AlignCenter)
        left_layout.addWidget(left_label)
        self.file_list_widget = QListWidget(self)
        self.file_list_widget.itemDoubleClicked.connect(self.fileListItemDoubleClicked)
        left_layout.addWidget(self.file_list_widget)
        left_container = QWidget()
        left_container.setLayout(left_layout)
        left_container.setMinimumWidth(150)
        main_layout.addWidget(left_container, 1)  # Poids 1 pour la colonne de gauche

        # Colonne centrale : Contrôles principaux
        center_layout = QVBoxLayout()
        self.connect_button = QPushButton("Connexion au Teensy", self)
        self.connect_button.clicked.connect(self.connectDevice)
        center_layout.addWidget(self.connect_button)

        self.mode_button = QPushButton("Mode Auto", self)
        self.mode_button.setCheckable(True)
        self.mode_button.clicked.connect(self.modeChanged)
        center_layout.addWidget(self.mode_button)

        # Encapsuler l'AngleWidget dans un layout pour le centrer verticalement
        angle_container_layout = QVBoxLayout()
        angle_container_layout.addStretch()  # espace flexible en haut
        self.angle_widget = AngleWidget(self)
        self.angle_widget.setFixedSize(200, 200)
        angle_container_layout.addWidget(self.angle_widget, alignment=Qt.AlignHCenter)
        angle_container_layout.addStretch()  # espace flexible en bas
        center_layout.addLayout(angle_container_layout)

        self.selected_angle_label = QLabel("Angle sélectionné: 0°", self)
        center_layout.addWidget(self.selected_angle_label)

        self.slider = QSlider(Qt.Horizontal, self)
        self.slider.setMinimum(0)
        self.slider.setMaximum(359)
        self.slider.setValue(0)
        self.slider.valueChanged.connect(self.angleChanged)
        center_layout.addWidget(self.slider)

        self.current_track_label = QLabel("Fichier en cours de lecture : Aucun", self)
        center_layout.addWidget(self.current_track_label)

        self.progress_bar = QProgressBar(self)
        self.progress_bar.setRange(0, 100)
        self.progress_bar.setValue(0)
        center_layout.addWidget(self.progress_bar)

        music_layout = QHBoxLayout()
        self.prev_button = QPushButton("Précédent", self)
        self.prev_button.clicked.connect(self.prevTrack)
        music_layout.addWidget(self.prev_button)
        self.pause_button = QPushButton("Pause", self)
        self.pause_button.setCheckable(True)
        self.pause_button.clicked.connect(self.pauseTrack)
        music_layout.addWidget(self.pause_button)
        self.next_button = QPushButton("Suivant", self)
        self.next_button.clicked.connect(self.nextTrack)
        music_layout.addWidget(self.next_button)
        center_layout.addLayout(music_layout)

        main_layout.addWidget(self.wrapLayout(center_layout), 2)  # Poids 2 pour la colonne centrale


        # Colonne de droite : Contrôle du volume
        right_layout = QVBoxLayout()
        volume_label = QLabel("Volume", self)
        volume_label.setAlignment(Qt.AlignCenter)
        right_layout.addWidget(volume_label)

        # Création d'un layout pour centrer le slider verticalement
        volume_slider_layout = QVBoxLayout()
        volume_slider_layout.addStretch()
        self.volume_slider = QSlider(Qt.Vertical, self)
        self.volume_slider.setMinimum(0)
        self.volume_slider.setMaximum(100)
        self.volume_slider.setValue(40)
        self.volume_slider.valueChanged.connect(self.volumeChanged)
        volume_slider_layout.addWidget(self.volume_slider, alignment=Qt.AlignHCenter)
        volume_slider_layout.addStretch()

        right_layout.addLayout(volume_slider_layout)

        self.volume_value_label = QLabel("40%", self)
        self.volume_value_label.setAlignment(Qt.AlignCenter)
        right_layout.addWidget(self.volume_value_label)

        main_layout.addWidget(self.wrapLayout(right_layout), 1)  # Poids 1 pour la colonne de droite


        container = QWidget()
        container.setLayout(main_layout)
        self.setCentralWidget(container)

    def wrapLayout(self, layout):
        """Encapsule un QLayout dans un QWidget."""
        widget = QWidget()
        widget.setLayout(layout)
        return widget

    def connectDevice(self):
        self.sendCommand("CONNECT")
        self.connect_button.setEnabled(False)
        self.interfaceConnected = True
        QTimer.singleShot(500, self.requestFileList)

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
        self.selected_angle_label.setText("Angle sélectionné: {}°".format(value))
        self.sendCommand("SET_ANGLE:{}".format(value))
        self.sendCommand("GET_ANGLE")

    def volumeChanged(self, value):
        self.volume_value_label.setText("{}%".format(value))
        self.sendCommand("VOLUME:{}".format(value))

    def sendGetAngle(self):
        if self.interfaceConnected and self.autoMode:
            self.sendCommand("GET_ANGLE")

    def requestFileList(self):
        self.file_list_entries = []
        self.file_list_widget.clear()
        self.sendCommand("GET_FILELIST")

    def fileListItemDoubleClicked(self, item):
        idx = item.data(Qt.UserRole)
        if idx is not None:
            self.sendCommand("PLAY_INDEX:{}".format(idx))

    def processIncomingMessages(self):
        if not self.interfaceConnected or not self.serial_port.isOpen():
            return
        while self.serial_port.in_waiting:
            try:
                line = self.serial_port.readline().decode('utf-8').strip()
            except Exception as e:
                print("Decoding error:", e)
                continue
            if line.startswith("GET_ANGLE:"):
                angle_value = line.split(":", 1)[1]
                try:
                    angle_int = int(angle_value)
                    self.angle_widget.setAngle(angle_int)
                except ValueError:
                    print("Invalid angle value:", angle_value)
            elif line.startswith("SET_ANGLE:"):
                angle_value = line.split(":", 1)[1]
                self.selected_angle_label.setText("Angle sélectionné: {}°".format(angle_value))
            elif line.startswith("TRACK:"):
                track_title = line.split(":", 1)[1]
                self.current_track_label.setText("Fichier en cours de lecture : {}".format(track_title))
                # Mise à jour de la sélection dans la liste de fichiers
                for i in range(self.file_list_widget.count()):
                    item = self.file_list_widget.item(i)
                    if item.text().strip() == track_title.strip():
                        self.file_list_widget.setCurrentItem(item)
                        break
            elif line.startswith("PROGRESS:"):
                progress_value = line.split(":", 1)[1]
                try:
                    progress_int = int(progress_value)
                    self.progress_bar.setValue(progress_int)
                except ValueError:
                    print("Invalid progress value:", progress_value)
            elif line.startswith("VOLUME:"):
                volume_value = line.split(":", 1)[1]
                self.volume_value_label.setText("{}%".format(volume_value))
            elif line.startswith("FILE:"):
                # Format : "FILE:index|filename"
                try:
                    data = line[5:]  # Supprimer "FILE:" du début
                    parts = data.split("|")
                    if len(parts) == 2:
                        idx, filename = parts
                        idx = int(idx)
                        item = QListWidgetItem(filename.strip())
                        item.setData(Qt.UserRole, idx)
                        self.file_list_widget.addItem(item)
                except Exception as e:
                    print("Error processing file list entry:", line, e)
            elif line == "FILELIST_END":
                pass
            else:
                print("Unknown message:", line)

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
