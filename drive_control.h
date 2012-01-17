#ifndef DRIVE_CONTROL_h
#define DRIVE_CONTROL_h

struct FileEntry {
  char name[11];
};

class DriveControl {
public:
  DriveControl(void(*getFileList)(int,int,FileEntry*), void(*mountFile)(int,int,int));

  void(*getFileList)(int,int,FileEntry*);
  void(*mountFile)(int,int,int);
};

#endif
