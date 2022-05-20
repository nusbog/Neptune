#include <fstream>
#include <string>

char *ReadContentFromFile(char *path) {
    char *buffer = 0;
    long length;
    FILE * f = fopen (path, "rb");

    if (f)
    {
      fseek (f, 0, SEEK_END);
      length = ftell (f) + 1;
      fseek (f, 0, SEEK_SET);
      buffer = (char*)malloc (length);
      if (buffer)
      {
        fread (buffer, 1, length, f);
      }
      fclose (f);
    } else {
        return NULL;
    }

    buffer[length - 1] = '\0';
    return buffer;
}
