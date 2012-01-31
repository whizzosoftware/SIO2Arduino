#ifndef DRIVE_CONTROL_h
#define DRIVE_CONTROL_h

struct FileEntry {
  char name[11];
  bool isDirectory;
};

class DriveControl {
public:
  DriveControl(int(*getFileList)(int,int,FileEntry*), void(*mountFile)(int,int), void(*changeDir)(int));

  int(*getFileList)(int,int,FileEntry*);
  void(*mountFile)(int,int);
  void(*changeDir)(int);
};

#endif
