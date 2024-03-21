#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import datetime
import os
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
from audioplayer import AudioPlayer
class MyEventHandler(FileSystemEventHandler):
    def on_any_event(self, event):
        if event.is_directory:
            return
        elif event.event_type == 'created':
            # Handle file creation event
            strP = f"File {event.src_path} has been created."
            print(strP)
            wd = WatchLog(strP)
            wd.WriteLog()
        elif event.event_type == 'modified':
            # Handle file modification event
            strP = f"File {event.src_path} has been modified."
            print(strP)
            wd = WatchLog(strP)
            wd.WriteLog()
        elif event.event_type == 'deleted':
            # Handle file deletion event
            strP = f"File {event.src_path} has been deleted."
            print(strP)
            wd = WatchLog(strP)
            wd.WriteLog()
        else:
            # Handle other event types
            strP = f"New event type: {event.event_type} for {event.src_path}"
            print(strP)
            wd = WatchLog(strP)
            wd.WriteLog()

class WatchLog:
    def __init__(self,strLog:str):
        current_time = datetime.datetime.now().strftime("%H:%M:%S")
        self.strLog = current_time + " " + strLog
    def WriteLog(self):
        strLogPath = "/Volumes/MacintoshHD2/WatchLog"
        strFileName = datetime.date.today().strftime("%Y-%m-%d") + ".txt"
        strFileToWrite = os.path.join(strLogPath, strFileName)
        # if not os.path.exists(strFileToWrite):
        #     with open(strFileToWrite,'w') as f:
        #         pass
        #     f.close()
        try:
            with open(strFileToWrite,mode='a') as f:
                f.write(self.strLog + "\n")
            f.close()
            self.PlayAlm()
        except:
            pass

    def PlayAlm(self):
        try:
            #AudioPlayer(os.getcwd() + "/alam.mp3").play(block=True)
            AudioPlayer("/Volumes/MacintoshHD2/WatchDog/__pycache__/alam.mp3").play(block=True)
        except:
            pass
if __name__ == "__main__":
    #get all folders to watch
    try:
        print('Watchdog is running...')
        strfolder = []
        with open("/Volumes/MacintoshHD2/WatchDog/__pycache__/folders2watch.txt",'r') as f:
            strfolder = f.read().splitlines()
        f.close()
        if len(strfolder) == 0:
            print('The folder list file is empty!')
    except:
        pass
    folders_to_track = strfolder #['/private/etc', '/Volumes/MacintoshHD2/FunProj']
    event_handler = MyEventHandler()
    observers = []
    for folder in folders_to_track:
        observer = Observer()
        observer.schedule(event_handler, folder, recursive=True)
        observer.start()
        observers.append(observer)

    try:
        while True:
            # Keep the observers alive
            pass
    except KeyboardInterrupt:
        for observer in observers:
            observer.stop()
        for observer in observers:
            observer.join()
