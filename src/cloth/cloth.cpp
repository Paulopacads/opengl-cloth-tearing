#include "cloth.hpp"
#include <algorithm>
#include <queue>

using namespace cgp;

void print_array(numarray<uint3> array, int width) {

    for (int i = 0; i < array.size(); i++) {
        printf(" | (%d, %d, %d) |", array[i][0], array[i][1], array[i][2]); 
        if (i % width == 0)
            printf("\n");
    }
    printf("\n");
}

int find_neighbor(const std::vector<vertex_infos> vertices, const int vertex, const int neighbor) {
    std::vector<spring> springs = vertices[vertex].springs;
    for (unsigned int i = 0; i < springs.size(); i ++) {
        if (springs[i].id == vertex)
            return i;
    }

    return -1;
}

void add_neighbor_vertex(std::vector<vertex_infos> *vertices, int vertex_a, int vertex_b, float len, bool is_diag) {
    
    if (vertex_a >= vertices->size() || vertex_b >= vertices->size() || vertex_a < 0 || vertex_b < 0)
        return;

    if (is_diag) {
        spring ab = {vertex_b, len}; 
        spring ba = {vertex_a, len};
        (*vertices)[vertex_a].springs.push_back(ab); 
        (*vertices)[vertex_b].springs.push_back(ba); 
    }
    else {
        // Avoid adding multiple times the same neighbor
        if (find_neighbor(*vertices, vertex_a, vertex_b) == -1) {
            spring ab = {vertex_b, len}; 
            spring ba = {vertex_a, len};
            (*vertices)[vertex_a].springs.push_back(ab); 
            (*vertices)[vertex_b].springs.push_back(ba); 
        }
    }
}

mesh generate_cloth(unsigned int height,unsigned int width, vec3 start_pos, float total_len_border, std::vector<vertex_infos> *vertices_infos, bool save_infos = false) {
    vec3 actual_pos = start_pos; 
    mesh shape; 

    float len_border = 1.0f / (static_cast<float>(width) - 1.0f);
    printf("Create new grid - H: %d, W: %d, Len border: %f\n", height, width, len_border);

    // Init all the vertices
    for (unsigned int i = 0; i < height; i ++) {
        actual_pos = start_pos; 
        actual_pos[1] += len_border * i;
        for (unsigned int j = 0; j < width; j ++) {
            
            float const u = static_cast<float>(i)/(static_cast<float>(height) -1.0f);
			float const v = static_cast<float>(j)/(static_cast<float>(width) -1.0f);

            if (save_infos) {
                vertex_infos vertex; 
                vertex.force = vec3(0, 0, 0); 
                vertex.velocity = vec3(0, 0, 0);
                vertex.springs = std::vector<spring>(); 
                vertices_infos->push_back(vertex); 
            }

            shape.position.push_back(actual_pos); 
            shape.normal.push_back(vec3(0, 0, 1));
            shape.uv.push_back({u, v});


            actual_pos[0] += len_border; 
        }

        actual_pos = start_pos; 
        actual_pos[1] += len_border * (static_cast<float>(i) + 0.5f);
        actual_pos[0] += len_border / 2.0f; 

        if (i < height - 1) {
            // Create intermediate vertex 
            for (unsigned int j = 0; j < width - 1; j++) {
                
                float const u = (static_cast<float>(i) + 0.5f) / (static_cast<float>(height)-1.0f);
                float const v = (static_cast<float>(j) + 0.5f) / (static_cast<float>(width)-1.0f);

                if (save_infos) {
                    vertex_infos vertex; 
                    vertex.springs = std::vector<spring>(); 
                    vertices_infos->push_back(vertex); 
                }

                shape.position.push_back(actual_pos); 
                shape.normal.push_back(vec3(0, 0, 1));
                shape.uv.push_back({u, v});

                actual_pos[0] += len_border; 
            }
        }
        
    }
    

    // Triangle connectivity 
    numarray<uint3> connectivity; 
    int actual_index = 0; 
    for (unsigned int i = 0; i < height - 1; i++) {
        actual_index += width; 
        for (unsigned int j = 0; j < width - 1; j++) {
            
            unsigned int top_right = actual_index - width + 1; 
            unsigned int top_left = actual_index - width; 
            unsigned int bot_left = actual_index + width - 1; 
            unsigned int bot_right = actual_index + width; 

            connectivity.push_back({actual_index, top_left, top_right}); 
            connectivity.push_back({actual_index, top_right, bot_right});
            connectivity.push_back({actual_index, bot_right, bot_left});
            connectivity.push_back({actual_index, bot_left, top_left});

            if (save_infos) {

                float len_diag = norm(shape.position[top_left] - shape.position[actual_index]);

                add_neighbor_vertex(vertices_infos, actual_index, top_left, len_diag, true);
                add_neighbor_vertex(vertices_infos, actual_index, top_right, len_diag, true);
                add_neighbor_vertex(vertices_infos, actual_index, bot_left, len_diag, true);
                add_neighbor_vertex(vertices_infos, actual_index, bot_right, len_diag, true);

                add_neighbor_vertex(vertices_infos, top_left, top_right, len_border, false);
                add_neighbor_vertex(vertices_infos, top_left, bot_left, len_border, false);
                add_neighbor_vertex(vertices_infos, top_right, bot_right, len_border, false);
                add_neighbor_vertex(vertices_infos, bot_left, bot_right, len_border, false);

                // Create double len springs
                /*float len2 = len_diag * 2;
                int width_int = static_cast<int>(width); 
                int double_top_right = actual_index - width_int - (width_int - 2); 
                int double_top_left = actual_index - (width_int * 2); 
                
                int double_bot_left = actual_index + width_int + (width_int - 2); 
                int double_bot_right = actual_index + width * 2; 
                //printf("Connect: %d, %d\n",  double_top_left, double_top_right)
                if (j < width - 2 && i != 0) 
                {
                    printf("Connect TR: %d, %d\n", actual_index, double_top_right);
                    add_neighbor_vertex(vertices_infos, actual_index, double_top_right, len2, false);
                }
                if (j >= 1 && i != 0)
                {
                    printf("Connect TL: %d, %d\n", actual_index, double_top_left);
                    add_neighbor_vertex(vertices_infos, actual_index, double_top_left, len2, false);
                }
                if (j < width - 2  && i < height - 2) 
                {
                    printf("Connect BR: %d, %d\n", actual_index, double_bot_right);
                    add_neighbor_vertex(vertices_infos, actual_index, double_bot_right, len2, false);
                }
                if (j >= 1 && i < height - 2) 
                {
                    printf("Connect BL: %d, %d\n", actual_index,  double_bot_left);
                    add_neighbor_vertex(vertices_infos, actual_index, double_bot_left, len2, false);
                }*/
            }

            actual_index ++;
        }
    }

    /* Add double springs to make a stronger cloth 
    if (save_infos) {
        actual_index = 0;
        for (unsigned int i = 0; i < height; i++) {
            for (unsigned int j = 0; j < width; j++) {
                int top_2 = actual_index - (3 * width + (width - 2));
                int bot_2 = actual_index + (3 * width + (width - 2));

                if (j >= 2) {
                    add_neighbor_vertex(vertices_infos, actual_index, actual_index - 2, len_border * 2, true);
                }
                if (j < width - 2) {
                    add_neighbor_vertex(vertices_infos, actual_index, actual_index + 2, len_border * 2, true);
                }
                add_neighbor_vertex(vertices_infos,  actual_index, top_2, len_border * 2, true);
                add_neighbor_vertex(vertices_infos,  actual_index, bot_2, len_border * 2, true);
                actual_index += 1;
            }
            actual_index += width - 1;
        }
    }*/
    

    //print_array(connectivity, width);
    //printf("%d\n", shape.position.size());
    shape.connectivity = connectivity; 
	shape.fill_empty_field();
	shape.flip_connectivity();

    return shape; 
}

void cloth_structure::initialize(int N_samples_edge_arg, float len_border_cloth, float start_height_cloth)
{
    assert_cgp(N_samples_edge_arg > 3, "N_samples_edge=" + str(N_samples_edge_arg) + " should be > 3");

    position.clear();
    normal.clear();
    vertices.clear();

    position.resize(N_samples_edge_arg * N_samples_edge_arg);
    normal.resize(N_samples_edge_arg * N_samples_edge_arg);
    vertices = std::vector<vertex_infos>();

    mesh const cloth_mesh = generate_cloth(N_samples_edge_arg, N_samples_edge_arg, { -(len_border_cloth/2.0f),0,start_height_cloth }, len_border_cloth, &vertices, true);

    position = cloth_mesh.position;
    normal = cloth_mesh.normal;
    triangle_connectivity = cloth_mesh.connectivity;
    shape = cloth_mesh;
}

void cloth_structure::update_normal()
{
    normal_per_vertex(position, triangle_connectivity, normal);
}

int cloth_structure::N_samples_edge() const
{
    return position.size();
}

int findTriangle(cgp::numarray<cgp::uint3> triangles,unsigned int a,unsigned int b) {
    for (unsigned int i = 0; i < triangles.size(); i ++)
    {
        if (triangles[i][0] == a || triangles[i][1] == a || triangles[i][2] == a) {
            if (triangles[i][0] == b || triangles[i][1] == b || triangles[i][2] == b)
            {
                return i; 
            }
        }
    }
    return -1;
}

int howMuchTriangles(cgp::numarray<cgp::uint3> triangles,unsigned int a,unsigned int b) {
    int found = 0;
    for (unsigned int i = 0; i < triangles.size(); i ++)
    {
        if (triangles[i][0] == a || triangles[i][1] == a || triangles[i][2] == a) {
            if (triangles[i][0] == b || triangles[i][1] == b || triangles[i][2] == b)
            {
                found ++;
            }
        }
    }
    return found;
}

void cloth_structure::update_triangles(unsigned int oldVerticeIndex,unsigned int newVerticeIndex, std::vector<spring> springsChanged) 
{
    for (unsigned int i = 0; i < springsChanged.size(); i ++) 
    {
        int neighboor = springsChanged[i].id;
        int found = findTriangle(triangle_connectivity, oldVerticeIndex, neighboor); 
        if (found == -1) {
            //printf("ERROR: Try to change a triangle that doesn't exist ! Index: %d, %d\n", oldVerticeIndex, neighboor); 
            continue;
        }

        if (triangle_connectivity[found][0] == oldVerticeIndex)
            triangle_connectivity[found][0] = newVerticeIndex;
        else if (triangle_connectivity[found][1] == oldVerticeIndex)
            triangle_connectivity[found][1] = newVerticeIndex;
        else 
            triangle_connectivity[found][2] = newVerticeIndex;
    }
    shape.uv.push_back(shape.uv[oldVerticeIndex]);
    shape.color.push_back(shape.color[oldVerticeIndex]);
}

bool cloth_structure::should_break(int vertex, std::vector<int>& neighboors) {
    int side_no_triangle = 0; 
    std::vector<spring> springs = vertices[vertex].springs;
    
    neighboors.push_back(springs[0].id); 
    
    for (int i = 0; i < springs.size(); i++) 
    {
        int actNeighboor = springs[i].id;
        int found = howMuchTriangles(triangle_connectivity, vertex, actNeighboor); 
        if (found == 0) 
        {
            printf("ERROR No Triangle found using %d, %d vertices !\n", vertex, actNeighboor); 
            continue;
        }
        else if (found == 1) 
            side_no_triangle ++;
    }

    std::queue<int> stack = std::queue<int>(); 
    stack.push(springs[0].id);
    while (stack.size() > 0) {
        int actVertex = stack.front();
        stack.pop();

        if (neighboors.end() != std::find(neighboors.begin(), neighboors.end(), actVertex)) 
        {
            continue;
        }
        neighboors.push_back(actVertex);
        std::vector<spring> springs = vertices[actVertex].springs;
        for (spring s : springs) 
        {
            for (spring s2 : vertices[s.id].springs) 
            {
                if (s2.id == vertex) 
                {
                    stack.push(s.id);
                }
            }
        }

    }

    return side_no_triangle >= 4;
}

void cloth_structure_drawable::initialize(int N_samples_edge, float len_border_cloth, float start_height_cloth)
{
    std::vector<vertex_infos> vert_inf = std::vector<vertex_infos>();
    mesh const cloth_mesh = generate_cloth(N_samples_edge, N_samples_edge, { -(len_border_cloth/2.0f),0, start_height_cloth }, len_border_cloth, &vert_inf, false);
    drawable.clear();
    drawable.initialize_data_on_gpu(cloth_mesh);
    drawable.material.phong.specular = 0.0f;
    opengl_check;
}


void cloth_structure_drawable::update(cloth_structure& cloth, cgp::opengl_texture_image_structure cloth_texture)
{    
    cloth.shape.position = cloth.position; 
    cloth.shape.normal = cloth.normal; 
    cloth.shape.connectivity = cloth.triangle_connectivity;
    
    drawable.clear();
    drawable.initialize_data_on_gpu(cloth.shape);
    drawable.material.phong.specular = 0.0f;
    drawable.texture = cloth_texture;
    //drawable.vbo_position.initialize_data_on_gpu(cloth.position.data);
    //drawable.vbo_position.update(cloth.position.data);
    //drawable.vbo_normal.initialize_data_on_gpu(cloth.normal.data);
    //drawable.vbo_normal.update(cloth.normal.data);
    //drawable.ebo_connectivity.initialize_data_on_gpu(cloth.triangle_connectivity);
    //drawable.material.phong.specular = 0.0f;
}

void draw(cloth_structure_drawable const& cloth_drawable, environment_generic_structure const& environment)
{
    draw(cloth_drawable.drawable, environment);
}
void draw_wireframe(cloth_structure_drawable const& cloth_drawable, environment_generic_structure const& environment)
{
    draw_wireframe(cloth_drawable.drawable, environment);
}

