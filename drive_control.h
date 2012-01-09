#ifndef DRIVE_CONTROL_h
#define DRIVE_CONTROL_h

struct FileEntry {
  char* name;
};

class DriveControl {
public:
  DriveControl(FileEntry**(*getFileList)(int,int), void(*mountFile)(int,int));

  FileEntry**(*getFileList)(int,int);
  void(*mountFile)(int,int);
};

#endif
