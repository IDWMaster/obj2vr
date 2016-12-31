#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <list>
#include <sstream>
#include <map>
#include "assimp/include/assimp/Importer.hpp"
#include "assimp/include/assimp/postprocess.h"
#include <assimp/scene.h>

class Vector2 {
public:
  float x;
  float y;
  Vector2() {
    x = 0;
    y = 0;
  }
  Vector2(float x, float y):x(x),y(y) {
  }
};
class Vector3 {
public:
  float x;
  float y;
  float z;
  Vector3() {
    x = 0;
    y = 0;
    z = 0;
  }
  Vector3(float x, float y, float z):x(x),y(y),z(z) {
    
  }
};


class GPUVertexDefinition {
public:
  aiVector3D vert;
  aiVector2D texcoord;
  aiVector3D normal;
};





int main(int argc, char** argv) {
  if(argc<3) {
    printf("USAGE: %s [objfilename] [outfilename]\n",argv[0]);
    return 0;
  }
  
  struct stat us; //MAC == Status symbol
  if(stat(argv[1],&us)) {
    printf("Unable to open input file %s\n",argv[1]);
    return -1;
  }
  Assimp::Importer importer;
  auto scene = importer.ReadFile(argv[1],aiProcessPreset_TargetRealtime_MaxQuality);
  
  
  int fd = open(argv[2],O_CREAT | O_RDWR,S_IRUSR | S_IWUSR);
  ftruncate(fd,0);
  unsigned char version = 0;
  write(fd,&version,1);
  for(size_t i = 0;i<scene->mNumMeshes;i++) {
    auto mesh = scene->mMeshes[i];
    uint16_t size = mesh->mName.length;
    write(fd,&size,sizeof(size));
    write(fd,mesh->mName.data,size);
    size_t facecount = mesh->mNumFaces;
    uint32_t numverts = facecount*3; //Since we've triangulated; each face has 3 vertices.
    write(fd,&numverts,sizeof(numverts));
    auto faces = mesh->mFaces;
    auto verts = mesh->mVertices;
    auto texcoords = mesh->mTextureCoords;
    auto normals = mesh->mNormals;
    
    for(size_t c = 0;c<facecount;c++ /*This is how C++ was invented*/) {
      for(size_t index = 0;index<3;index++) {
	GPUVertexDefinition def;
	def.vert = verts[faces[c].mIndices[index]];
	def.texcoord = aiVector2D(texcoords[0][faces[c].mIndices[index]].x,texcoords[0][faces[c].mIndices[index]].y);
	def.normal = normals[faces[c].mIndices[index]];
	write(fd,&def,sizeof(def));
      }
    }
  }
  close(fd);
  
}
