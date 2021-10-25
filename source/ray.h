/* date = October 25th 2021 7:33 pm */

#ifndef RAY_H
#define RAY_H
#include <cstdlib>


struct Ray {
    v3 origin;
    v3 direction;
};

struct Hit_record {
    v3 p;
    v3 normal;
    f32 t;
    bool front_face;
};

struct Sphere {
    v3 center;
    f32 radius;
};

struct Sphere_world {
    u32 sphere_index;
    Sphere sphere_list[256];
};

struct Camera {
    v3 origin;
    v3 lower_left_corner;
    v3 horizontal;
    v3 vertical;
};

inline 
float random_f32() {
    return rand() / (RAND_MAX + 1.0f);
}

inline 
float random_f32(float min, float max) {
    return min + (max - min) * random_f32();
}

inline
v3 random_v3() {
    v3 result;
    
    result = {random_f32(), random_f32(), random_f32()};
    
    return result;
}

inline
v3 random_v3(f32 min, f32 max) {
    v3 result;
    
    result = {
        random_f32(min, max), 
        random_f32(min, max), 
        random_f32(min, max)
    };
    
    return result;
}

v3 random_in_unit_sphere() {
    while (true) {
        v3 p = random_v3(-1.0f, 1.0f);
        
        if (length_squared_v3(p) >= 1) 
            continue;
        
        return p;
    }
}

internal
void init_camera(Camera* camera) {
    f32 aspect_ratio = 16.0f / 9.0f;
    f32 viewport_height = 2.0f;
    f32 viewport_width = aspect_ratio * viewport_height;
    f32 focal_length = 1.0f;
    
    camera->origin = {};
    camera->horizontal = { viewport_width, 0, 0};
    camera->vertical = {0, viewport_height, 0};
    camera->lower_left_corner = camera->origin - camera->horizontal/2 - camera->vertical/2 - v3{0, 0, focal_length};
}

inline
Ray get_ray(Camera* camera, f32 u, f32 v) {
    return Ray {
        camera->origin, 
        camera->lower_left_corner + u * camera->horizontal + v * camera->vertical - camera->origin
    };
}

inline
v3 write_color(v3 color, i32 samples_per_pixel) {
    v3 result = {};
    
    f32 scale = 1.0f / samples_per_pixel;
    result.r = sqrtf(scale * color.r);
    result.g = sqrtf(scale * color.g);
    result.b = sqrtf(scale * color.b);
    
    result.r = clamp_f32(color.r);
    result.g = clamp_f32(color.g);
    result.b = clamp_f32(color.b);
    
    return result;
}
// P(t) = A + tb
// P - position
// A - ray origin
// b - ray direction
// t - "time"
inline
v3 ray_at(Ray* ray, f32 t) {
    v3 result;
    
    result = ray->origin + t * ray->direction;
    
    return result;
}

inline
void set_face_normal(Ray* ray, Hit_record* hit_record, v3 outward_normal) {
    hit_record->front_face = dot_v3(ray->direction, outward_normal) < 0;
    hit_record->normal = hit_record->front_face ? outward_normal :-outward_normal;
}

internal
bool hit_sphere(Sphere* sphere, Ray* ray, f32 t_min, f32 t_max, Hit_record* hit_record) {
    v3 oc = ray->origin - sphere->center;
    f32 a = length_squared_v3(ray->direction);
    f32 half_b = dot_v3(oc, ray->direction);
    f32 c = length_squared_v3(oc) - square(sphere->radius);
    auto discriminant = half_b*half_b - a * c;
    
    if (discriminant < 0)
        return false;
    
    f32 squared_val = sqrtf(discriminant);
    
    f32 root = (-half_b - squared_val) / a;
    
    if (root < t_min || t_max < root) {
        root = (-half_b + squared_val) / a;
        
        if (root < t_min || t_max < root)
            return false;
    }
    
    hit_record->t = root;
    hit_record->p = ray_at(ray, hit_record->t);
    v3 outward_normal = (hit_record->p - sphere->center) / sphere->radius;
    set_face_normal(ray, hit_record, outward_normal);
    return true;
}

bool find_first_hit(Sphere_world* world, Ray* ray, f32 t_min, f32 t_max, Hit_record* hit_record) {
    Hit_record temp_rec = {};
    bool hit_anything = false;
    f32 closest = t_max;
    
    for (u32 sphere_index = 0; sphere_index < world->sphere_index; sphere_index++) {
        bool hit = hit_sphere(&world->sphere_list[sphere_index], ray, t_min, closest, &temp_rec);
        if (hit) {
            hit_anything = true;
            closest = temp_rec.t;
            hit_record->p = temp_rec.p;
            hit_record->normal = temp_rec.normal;
            hit_record->t = temp_rec.t;
            hit_record->front_face = temp_rec.front_face;
        }
    }
    
    return hit_anything;
}

v3 ray_color(Sphere_world* world, Ray* ray, u32 depth) {
    Hit_record hit_record;
    
    if (depth <= 0)
        return v3 {0, 0, 0};
    
    bool found_first = find_first_hit(world, ray, 0, INFINITY, &hit_record);
    
    if (found_first) {
        v3 target = hit_record.p + hit_record.normal + random_in_unit_sphere();
        Ray temp_ray = {hit_record.p, target - hit_record.p};
        return 0.5f * ray_color(world, &temp_ray, depth - 1);
    }
    
    v3 unit_direction = unit_vector_v3(ray->direction);
    f32 t = 0.5f * (unit_direction.y + 1.0f);
    
    return (1.0f - t) * v3{1.0, 1.0, 1.0} + t * v3{0.5f, 0.7f, 1.0f};
}

void ray_udpate(Game_bitmap_buffer* bitmap_buffer) {
    // image
    f32 aspect_ratio = 16.0f / 9.0f;
    u32 image_width  = 400;
    u32 image_height = i32(image_width / aspect_ratio);
    u32 samples_per_pixel = 1;
    u32 max_depth = 5;
    
    // world
    local_persist Sphere_world world;
    
    // camera
    Camera camera = {};
    init_camera(&camera);
    
    world.sphere_list[world.sphere_index++] = Sphere {v3 {0, 0, -1}, 0.5};
    world.sphere_list[world.sphere_index++] = Sphere {v3 {0, -100.5, -1}, 100};
    
    // render
    for (i32 j = image_height - 1; j >= 0; j--) {
        char buffer[256];
        _snprintf_s(buffer, sizeof(buffer), "remaining: %d\n", j); 
        OutputDebugStringA(buffer);
        for (u32 i = 0; i < image_width; i++) {
            v3 temp_color = {};
            for (u32 sample_index = 0; sample_index < samples_per_pixel; sample_index++) {
                f32 u = (f32)(i + random_f32()) / (image_width - 1);
                f32 v = (f32)(j + random_f32()) / (image_height - 1);
                Ray ray = get_ray(&camera, u, v);
                
                temp_color += ray_color(&world, &ray, max_depth);
            }
            v3 color = {};
            color = write_color(temp_color, samples_per_pixel);
            draw_pixel(bitmap_buffer, v2 { (f32)i - image_width / 2, (f32)j - image_height / 2 }, color);
        }
    }
}

#endif //RAY_H
