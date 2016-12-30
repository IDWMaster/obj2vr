#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <list>
#include <sstream>
#include <map>

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
  Vector3 vert;
  Vector2 texcoord;
  Vector3 normal;
};

class Mesh {
public:
  std::vector<Vector3> verts;
  std::vector<Vector2> texcoords;
  std::vector<Vector3> normals;
  std::list<GPUVertexDefinition> faces;
  
  //Number of vertices
  size_t size = 0;
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
  int fd = open(argv[1],O_RDONLY);
  char* mander = (char*)mmap(0,us.st_size,PROT_READ, MAP_SHARED, fd, 0);
  
  char* ptr = mander;
  //Parse OBJ file
  auto expect = [&](const char* actors, char& found){
    while(ptr<mander+us.st_size) {
      for(size_t i = 0;actors[i] != 0;i++) {
	if(*ptr == actors[i]) {
	  found = *ptr;
	  ptr++;
	  return true;
	}
      }
      ptr++;
    }
    return false;
  };
  auto readWhitespace = [&](){
    while(ptr<mander+us.st_size && isspace(*ptr)){ptr++;}
  };
  
  auto readFloat = [&](){
    std::stringstream ss;
    while(ptr<mander+us.st_size && (isdigit(*ptr) || *ptr == '.' || *ptr == '-')) {ss<<*ptr;ptr++;}
    return atof(ss.str().data());
  };
  auto readInt = [&](){
    std::stringstream ss;
    while(ptr<mander+us.st_size && isdigit(*ptr)) {ss<<*ptr;ptr++;}
    return atoi(ss.str().data());
  };
  auto readIdentifier = [&](){
    std::stringstream ss;
    while(ptr<mander+us.st_size && !isspace(*ptr)){ss<<*ptr;ptr++;}
    return ss.str();
  };
  auto readLine = [&](){
    std::stringstream ss;
    while(ptr<mander+us.st_size && *ptr != '\n'){ss<<*ptr;ptr++;}
    return ss.str();
  };
  std::map<std::string,Mesh> meshes;
  
  std::string name = "root";
  Mesh current;
  
  while(ptr<mander+us.st_size) {
    readWhitespace();
    char s = *ptr;
    ptr++;
    readWhitespace();
  switch(s) {
    case '#':
      //Comment.
    {
      char found;
      expect("\n",found);
    }
      break;
    case 'v':
    {
      unsigned char n = *ptr;
      ptr++;
      readWhitespace();
      switch(n) {
	case 't':
	  //texture coordinate
	{
	    Vector2 texcoord;
	    texcoord.x = readFloat();
	    readWhitespace();
	    texcoord.y = readFloat();
	    current.texcoords.push_back(texcoord);
	}
	  break;
	case 'n':
	  //Normal
	{
	  Vector3 normal;
	  normal.x = readFloat();
	  readWhitespace();
	  normal.y = readFloat();
	  readWhitespace();
	  normal.z = readFloat();
	  current.normals.push_back(normal);
	  
	}
	  break;
	default:
	  //Vertex
	{
	  
	  Vector3 vertex;
	  vertex.x = readFloat();
	  readWhitespace();
	  vertex.y = readFloat();
	  readWhitespace();
	  vertex.z = readFloat();
	  current.verts.push_back(vertex);
	  
	}
	  break;
      }
    }
      break;
	case 'o':
	  meshes[name] = current;
	  current = Mesh();
	  name = readIdentifier();
	  break;
	case 'f':
	{
	  //Face
	  while(*(ptr-1) != '\n' && ptr<mander+us.st_size) {
	    
	    GPUVertexDefinition def;
	    int vertidx = readInt()-1;
	    if(vertidx<0 || vertidx>=current.verts.size()) {
	      printf("Error reading face attribute. Index %i is out of range of vertex buffer.\n",vertidx);
	      return -1;
	    }
	    def.vert = current.verts[vertidx];
	    ptr++;
	    vertidx = readInt()-1;
	    if(vertidx<0 || vertidx>=current.texcoords.size()) {
	      printf("Error reading face attribute. Index %i is out of range of texcoord buffer.\n",vertidx);
	      return -1;
	    }
	    def.texcoord = current.texcoords[vertidx];
	    ptr++;
	    vertidx = readInt()-1;
	    if(vertidx<0 || vertidx>=current.normals.size()) {
	      printf("Error reading face attribute. Index %i is out of range of normal buffer.\n",vertidx);
	      return -1;
	    }
	    def.normal = current.normals[vertidx];
	    ptr++;
	    current.faces.push_back(def);
	    current.size++;
	  }
	}
	  break;
	default:
	  readLine();
	  break;
  }
  }
  
  meshes[name] = current;
  //Output stage:
  munmap(mander,us.st_size);
  close(fd);
  fd = open(argv[2],O_CREAT | O_RDWR,S_IRUSR | S_IWUSR);
  unsigned char version = 0;
  write(fd,&version,1);
  for(auto bot = meshes.begin();bot != meshes.end();bot++) {
    uint16_t size = bot->first.size();
    write(fd,&size,sizeof(size));
    write(fd,bot->first.data(),size);
    auto mobile = bot->second.faces.begin();
    uint32_t usize = bot->second.size;
    write(fd,&usize,sizeof(usize));
    for(size_t i = 0;i<bot->second.size;i++) {
      GPUVertexDefinition def = *mobile;
      mobile++;
      write(fd,&def,sizeof(def));
    }
  }
  close(fd);
  
}
