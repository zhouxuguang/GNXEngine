#include "Intersection.h"
#include <cstdio>
#include <limits>
#include <random>
#include <type_traits>

using namespace mathutil;
namespace {
int checks=0,failures=0;

template<class T> void Check(const char* name, bool actual, bool expected)
{
    ++checks;
    if (actual!=expected) {
        if (failures<32)
            std::printf("FAIL %s/%s: expected %d, actual %d\n",
                std::is_same_v<T,float>?"float":"double",name,expected,actual);
        ++failures;
    }
}

// Independent reference: intersect the ray with each of the six faces.
// This does not repeat the slab interval algorithm used in the engine.
template<class T> bool FaceReference(const Ray<T>& ray,const AxisAlignedBox<T>& box)
{
    const auto& origin=ray.mOrigin;
    const auto& direction=ray.mDirection;
    if (origin.x>=box.minimum.x && origin.x<=box.maximum.x &&
        origin.y>=box.minimum.y && origin.y<=box.maximum.y &&
        origin.z>=box.minimum.z && origin.z<=box.maximum.z) return true;
    for (int axis=0;axis<3;++axis) {
        if (direction[axis]==0) continue;
        for (T face:{box.minimum[axis],box.maximum[axis]}) {
            const long double t=(static_cast<long double>(face)-origin[axis])/direction[axis];
            if (t<0) continue;
            bool onFace=true;
            for (int other=0;other<3;++other) if (other!=axis) {
                const long double p=origin[other]+t*direction[other];
                onFace=onFace && p>=box.minimum[other] && p<=box.maximum[other];
            }
            if (onFace) return true;
        }
    }
    return false;
}

template<class T> void Test()
{
    using V=Vector3<T>; using B=AxisAlignedBox<T>;
    const B unit(V(-1,-1,-1),V(1,1,1));
    auto check=[&](const char* name,V origin,V direction,bool expected,const B& box) {
        Check<T>(name,IntersectRayAABB(Ray<T>(origin,direction),box),expected);
    };
    for (int axis=0;axis<3;++axis) for (T sign:{T(-1),T(1)}) {
        V origin,direction;
        origin[axis]=T(3)*sign; direction[axis]=-sign;
        check("enter from each face",origin,direction,true,unit);
        check("box behind ray",origin,-direction,false,unit);
        V alongFace,parallel;
        alongFace[axis]=sign;
        alongFace[(axis+1)%3]=-3;
        parallel[(axis+1)%3]=1;
        check("parallel on each face",alongFace,parallel,true,unit);
    }
    check("parallel outside slab",V(-3,2,0),V(1,0,0),false,unit);
    check("start inside",V(),V(1,0,0),true,unit);
    check("start on face pointing away (t=0)",V(-1,0,0),V(-1,0,0),true,unit);
    check("parallel on minimum face",V(-3,-1,0),V(1,0,0),true,unit);
    check("parallel on maximum face",V(-3,1,0),V(1,0,0),true,unit);
    check("parallel on edge",V(-3,1,1),V(1,0,0),true,unit);
    check("corner diagonal",V(-3,-3,-3),V(1,1,1),true,unit);
    check("touch one edge at one parameter",V(-2,0,0),V(1,1,0),true,unit);
    check("disjoint face intervals",V(-3,2,0),V(1,1,0),false,unit);
    check("zero direction inside",V(),V(),true,unit);
    check("zero direction outside",V(2,0,0),V(),false,unit);
    check("zero direction on boundary",V(1,1,1),V(),true,unit);
    check("negative zero on boundary",V(-3,-1,0),V(1,-T(0),0),true,unit);
    const B plane(V(-1,-1,0),V(1,1,0));
    check("zero thickness plane hit",V(0,0,2),V(0,0,-1),true,plane);
    check("zero thickness plane parallel miss",V(0,0,2),V(1,0,0),false,plane);
    const B point(V(2,3,4),V(2,3,4));
    check("point box hit",V(0,3,4),V(1,0,0),true,point);
    check("point box miss",V(0,3,5),V(1,0,0),false,point);
    const B line(V(1,-2,3),V(1,2,3));
    check("line box hit",V(0,0,3),V(1,0,0),true,line);
    check("line box miss",V(0,0,4),V(1,0,0),false,line);
    for (T magnitude:{T(100),T(1e-15),std::numeric_limits<T>::min()}) {
        check("scaled nonzero direction hit",V(-3,0,0),V(magnitude,0,0),true,unit);
        check("scaled nonzero direction away",V(-3,0,0),V(-magnitude,0,0),false,unit);
        check("scaled nonzero direction miss",V(-3,2,0),V(magnitude,0,0),false,unit);
    }
    B modified=unit;
    modified.minimum=V(10,20,30); modified.maximum=V(12,24,36);
    check("public bounds changed without refreshing caches",V(0,22,33),V(1,0,0),true,modified);
    check("old cached box must not hit",V(-3,0,0),V(1,0,0),false,modified);
    const T huge=std::numeric_limits<T>::max()*T(.75);
    const B large(V(-huge,-1,-1),V(huge,1,1));
    check("large bounds hit despite length overflow",V(),V(1,0,0),true,large);
    check("large bounds parallel miss",V(0,2,0),V(1,0,0),false,large);

    std::mt19937_64 random(0xAABB2026);
    std::uniform_real_distribution<double> coordinate(-10,10),extent(.1,4);
    for (int sample=0;sample<10000;++sample) {
        const V center(T(coordinate(random)),T(coordinate(random)),T(coordinate(random)));
        const V half(T(extent(random)),T(extent(random)),T(extent(random)));
        const B box(center-half,center+half);
        const V origin(T(coordinate(random)),T(coordinate(random)),T(coordinate(random)));
        V direction(T(coordinate(random)),T(coordinate(random)),T(coordinate(random)));
        if (sample%3==0) direction[sample%3]=0;
        if (sample%5==0) direction[(sample+1)%3]=0;
        const Ray<T> ray(origin,direction);
        Check<T>("random six-face reference",IntersectRayAABB(ray,box),FaceReference(ray,box));
    }
}
}

int main()
{
    Test<float>(); Test<double>();
    std::printf("IntersectRayAABB: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
