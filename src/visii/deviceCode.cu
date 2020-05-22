#include "deviceCode.h"
#include "launchParams.h"
#include <optix_device.h>
#include <owl/common/math/random.h>
typedef owl::common::LCG<4> Random;

extern "C" __constant__ LaunchParams optixLaunchParams;

struct TriMeshPayload {
    float r = -1.f, g = -1.f, b = -1.f, tmax = -1.f;
};

inline __device__
vec3 missColor(const owl::Ray &ray)
{
  auto pixelID = owl::getLaunchIndex();

  vec3 rayDir = glm::normalize(glm::vec3(ray.direction.x, ray.direction.y, ray.direction.z));
  float t = 0.5f*(rayDir.y + 1.0f);
  vec3 c = (1.0f - t) * vec3(1.0f, 1.0f, 1.0f) + t * vec3(0.5f, 0.7f, 1.0f);
  return c;
}

OPTIX_MISS_PROGRAM(miss)()
{
}

OPTIX_CLOSEST_HIT_PROGRAM(TriangleMesh)()
{
    TriMeshPayload &prd = owl::getPRD<TriMeshPayload>();

    const TrianglesGeomData &self = owl::getProgramData<TrianglesGeomData>();
    float2 bc = optixGetTriangleBarycentrics();

    // compute normal:
    const int   instID = optixGetInstanceIndex();
    const int   primID = optixGetPrimitiveIndex();
    const ivec3 index  = self.index[primID];
    const vec3 &A     = self.vertex[index.x];
    const vec3 &B     = self.vertex[index.y];
    const vec3 &C     = self.vertex[index.z];

    vec3 vcol;

    // temporary code...
    uint32_t entityID =  optixLaunchParams.instanceToEntityMap[instID];

    if (entityID >= MAX_ENTITIES) vcol = vec3(1.0f, 0.0f, 1.0f);
    else {
        EntityStruct entity = optixLaunchParams.entities[entityID];
        
        if ((entity.material_id < 0) || (entity.material_id >= MAX_MATERIALS)) {
            vcol = vec3(0.f, 1.0f, 0.0f);
        } else {
            MaterialStruct material = optixLaunchParams.materials[entity.material_id];
            vcol = vec3(material.base_color);
        }
    }


    // const vec3 &ACol = vec3(material.base_color); ///(self.colors == nullptr) ? vec3(optixLaunchParams.tri_mesh_color) : self.colors[index.x];
    // const vec3 &BCol = vec3(material.base_color); ///(self.colors == nullptr) ? vec3(optixLaunchParams.tri_mesh_color) : self.colors[index.y];
    // const vec3 &CCol = vec3(material.base_color); ///(self.colors == nullptr) ? vec3(optixLaunchParams.tri_mesh_color) : self.colors[index.z];
    vec3 Ng;
    if (self.normals) {
        const vec3 &NA = self.normals[index.x];
        const vec3 &NB = self.normals[index.y];
        const vec3 &NC = self.normals[index.z];
        Ng = NA * (1.f - (bc.x + bc.y)) + NB * bc.x + NC * bc.y;
    } else {
        Ng = normalize(cross(B-A,C-A));
    }

    auto rayDir = optixGetWorldRayDirection();
    vec3 dir = vec3(rayDir.x, rayDir.y, rayDir.z);

    // vec3 vcol = ACol * (1.f - (bc.x + bc.y)) + BCol * bc.x + CCol * bc.y;

    vec3 color = (.2f + .8f*fabs(dot(dir,Ng)))*vcol;
    prd.r = color.x;
    prd.g = color.y;
    prd.b = color.z;
    prd.tmax = optixGetRayTmax();
}

inline __device__
bool loadCamera(EntityStruct &cameraEntity, CameraStruct &camera, TransformStruct &transform)
{
    cameraEntity = optixLaunchParams.cameraEntity;
    if (!cameraEntity.initialized) return false;
    if ((cameraEntity.transform_id < 0) || (cameraEntity.transform_id >= MAX_TRANSFORMS)) return false;
    if ((cameraEntity.camera_id < 0) || (cameraEntity.camera_id >= MAX_CAMERAS)) return false;
    camera = optixLaunchParams.cameras[cameraEntity.camera_id];
    transform = optixLaunchParams.transforms[cameraEntity.transform_id];
    return true;
}

inline __device__
owl::Ray generateRay(const CameraStruct &camera, const TransformStruct &transform, ivec2 pixelID, ivec2 frameSize)
{
    /* Generate camera rays */    
    mat4 camWorldToLocal = transform.worldToLocal;
    mat4 projinv = camera.projinv;//glm::inverse(glm::perspective(.785398, 1.0, .1, 1000));//camera.projinv;
    mat4 viewinv = camera.viewinv * camWorldToLocal;
    vec2 inUV = vec2(pixelID.x, pixelID.y) / vec2(optixLaunchParams.frameSize);
    // if (optixLaunchParams.zoom > 0.f) {
    //     inUV /= optixLaunchParams.zoom;
    //     inUV += (.5f - (.5f / optixLaunchParams.zoom));
    // }

    vec3 origin = vec3(viewinv * vec4(0.f,0.f,0.f,1.f));

    vec2 dir = inUV * 2.f - 1.f; dir.y *= -1.f;
    vec4 t = (projinv * vec4(dir.x, dir.y, -1.f, 1.f));
    vec3 target = vec3(t) / float(t.w);
    vec3 direction = vec3(viewinv * vec4(target, 0.f));
    direction = normalize(direction);

    owl::Ray ray;
    ray.tmin = .0f;
    ray.tmax = 1e38f;//10000.0f;
    ray.origin = owl::vec3f(origin.x, origin.y, origin.z);
    ray.direction = owl::vec3f(direction.x, direction.y, direction.z);
    // ray.direction = owl::vec3f(0.0, 1.0, 0.0); // testing...
    // if ((pixelID.x == 0) && (pixelID.y == 0)) {
    //     // printf("dir: %f %f %f\n", ray.direction.x, ray.direction.y, ray.direction.z);
    //     printf("viewinv: %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n", 
    //         viewinv[0][0], viewinv[0][1], viewinv[0][2], viewinv[0][3],
    //         viewinv[1][0], viewinv[1][1], viewinv[1][2], viewinv[1][3],
    //         viewinv[2][0], viewinv[2][1], viewinv[2][2], viewinv[2][3],
    //         viewinv[3][0], viewinv[3][1], viewinv[3][2], viewinv[3][3]
    //     );
    // }

    ray.direction = normalize(owl::vec3f(direction.x, direction.y, direction.z));
    // ray.direction = normalize(owl::vec3f(target.x, target.y, target.z));

    // vec3 lookFrom = origin;//(-4.f,-3.f,-2.f);
    // vec3 lookAt(0.f,0.f,0.f);
    // vec3 lookUp(0.f,0.f,1.f);
    // float cosFovy = 0.66f;
    // vec3 camera_pos = lookFrom;
    // vec3 camera_d00
    //   = normalize(lookAt-lookFrom);
    // float aspect = frameSize.x / float(frameSize.y);
    // vec3 camera_ddu
    //   = cosFovy * aspect * normalize(cross(camera_d00,lookUp));
    // vec3 camera_ddv
    //   = cosFovy * normalize(cross(camera_ddu,camera_d00));
    // camera_d00 -= 0.5f * camera_ddu;
    // camera_d00 -= 0.5f * camera_ddv;

    // direction 
    // = normalize(camera_d00
    //             + inUV.x * camera_ddu
    //             + inUV.y * camera_ddv);
    // ray.direction = owl::vec3f(direction.x, direction.y, direction.z);
    return ray;
}

OPTIX_RAYGEN_PROGRAM(rayGen)()
{
    auto pixelID = ivec2(owl::getLaunchIndex()[0], owl::getLaunchIndex()[1]);
    auto fbOfs = pixelID.x+optixLaunchParams.frameSize.x* ((optixLaunchParams.frameSize.y - 1) -  pixelID.y);
    Random random; random.init(pixelID.x/* + offset*/, pixelID.y/* + offset*/);

    EntityStruct    camera_entity;
    TransformStruct camera_transform;
    CameraStruct    camera;
    if (!loadCamera(camera_entity, camera, camera_transform)) {
        optixLaunchParams.fbPtr[fbOfs] = vec4(random(), random(), random(), 1.f);
        return;
    }

    owl::Ray ray = generateRay(camera, camera_transform, pixelID, optixLaunchParams.frameSize);

    vec3 finalColor = vec3(0.f);

    /* Intersect mesh */
    TriMeshPayload tprd;
    owl::traceRay(  /*accel to trace against*/ optixLaunchParams.world,
                    /*the ray to trace*/ ray,
                    /*prd*/ tprd);  
    
    if (tprd.tmax > 0.f) {
        finalColor = vec3(tprd.r, tprd.g, tprd.b);
    }
    // finalColor = vec3(ray.direction.x, ray.direction.y, ray.direction.z);
    /* Write AOVs */
    optixLaunchParams.fbPtr[fbOfs] = vec4(finalColor.r, finalColor.g, finalColor.b, 1.f);
}
