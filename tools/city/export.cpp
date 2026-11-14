#include "City.h"
#include "renderer/Renderer.h"
#include "renderer/PrimitiveMeshes.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <tuple>
#include <vector>

struct Source { std::vector<float> vertices; GLenum primitive; };
static std::vector<Source> sources;
using Key = std::tuple<GLenum,float,float,float,float>;
static std::map<Key,std::vector<float>> batches;
static void capture(const std::vector<float>& vertices, GLenum primitive, const Matrix4& model, Color color)
{
    auto& dest = batches[{primitive,color.r,color.g,color.b,color.a}];
    const float* m = model.data();
    for (std::size_t i=0; i<vertices.size(); i+=3) {
        const float x=vertices[i];
        const float y=vertices[i+1];
        const float z=vertices[i+2];
        dest.push_back(m[0]*x+m[4]*y+m[8]*z+m[12]);
        dest.push_back(m[1]*x+m[5]*y+m[9]*z+m[13]);
        dest.push_back(m[2]*x+m[6]*y+m[10]*z+m[14]);
    }
}
bool Renderer::uploadStaticMesh(const std::vector<float>& positions, GLenum primitive, std::size_t& id)
{
    id=sources.size(); sources.push_back({positions,primitive}); return true;
}
void Renderer::drawStaticMesh(std::size_t id, const Matrix4& model, Color color) const
{
    capture(sources.at(id).vertices,sources.at(id).primitive,model,color);
}
void Renderer::draw(MeshId id, const Matrix4& model, Color color) const
{
    switch(id) {
    case MeshId::Cube: capture(PrimitiveMeshes::cube(),GL_TRIANGLES,model,color); break;
    case MeshId::Quad: capture(PrimitiveMeshes::quad(),GL_TRIANGLES,model,color); break;
    case MeshId::Disc: capture(PrimitiveMeshes::disc(),GL_TRIANGLES,model,color); break;
    case MeshId::Line: capture(PrimitiveMeshes::line(),GL_LINES,model,color); break;
    case MeshId::Point: capture(PrimitiveMeshes::point(),GL_POINTS,model,color); break;
    case MeshId::Cone: capture(PrimitiveMeshes::cone(),GL_TRIANGLES,model,color); break;
    }
}
template<typename T> void write(std::ofstream& out,const T& value) { out.write(reinterpret_cast<const char*>(&value),sizeof(value)); }
int main(int argc, char** argv)
{
    if (argc != 3) { std::cerr << "Usage: city-export MESH COLLISION-RAW\n"; return 1; }
    Renderer renderer;
    City city;
    if (!city.initialize(renderer)) return 1;
    city.render(renderer);
    std::ofstream out(argv[1],std::ios::binary);
    out.write("CITYMESH",8);
    const std::uint32_t version=1;
    const std::uint32_t count=static_cast<std::uint32_t>(batches.size());
    write(out,version); write(out,count);
    std::size_t vertices=0;
    for (const auto& [key,data] : batches) {
        const auto [mode,r,g,b,a]=key;
        const std::uint32_t primitive=mode;
        const std::uint32_t vertexCount=static_cast<std::uint32_t>(data.size()/3);
        write(out,primitive); write(out,vertexCount);
        write(out,r); write(out,g); write(out,b); write(out,a);
        out.write(reinterpret_cast<const char*>(data.data()),data.size()*sizeof(float));
        vertices += vertexCount;
    }
    if (!out) return 1;
    std::ofstream collision(argv[2], std::ios::binary);
    const auto bounds = city.collisionBounds();
    const std::uint32_t boxCount = static_cast<std::uint32_t>(bounds.size());
    write(collision, boxCount);
    for (const Aabb& box : bounds) {
        write(collision, box.min.x); write(collision, box.min.y); write(collision, box.min.z);
        write(collision, box.max.x); write(collision, box.max.y); write(collision, box.max.z);
    }
    std::cout << "batches=" << count << " vertices=" << vertices << " collision_boxes=" << boxCount << '\n';
    return collision ? 0 : 1;
}
