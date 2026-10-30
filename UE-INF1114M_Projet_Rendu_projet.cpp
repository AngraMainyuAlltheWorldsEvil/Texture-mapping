// idem tp1, mais lecture du mesh avec MeshIOData

#include "vec.h"
#include "mat.h"

#include "glcore.h"
#include "mesh_io.h"
#include "buffers.h"
#include "program.h"
#include "uniforms.h"
#include "default_program.h"
#include "draw.h"
#include "texture.h"
#include <cmath>
#include "Eigen/Dense"
#include "Eigen/Sparse"

#include "window.h"
#include <set>

GLuint vao_applat = 0;
GLuint vao_LSM =0;
GLuint vao_LSM_applat=0;
bool applat  = false;
bool LSM=true;
float zoom      = 10.0f;
float offset_x   = 0.0f;
float offset_y   = 0.0f;
float zoomFactor = -5.0f; 
GLuint vao= 0;
unsigned count= 0;
GLuint border_vao;
GLuint interior_vao;
float interior_count;
float border_count;
using edge = std::pair<unsigned int, unsigned int>;

void init_edge_tab(std::set<edge>& edges, const MeshIOData& data)
{
    for(int i = 0; i < data.indices.size(); i += 3)
    {
        edge e1(data.indices[i], data.indices[i+1]);
        edge e2(data.indices[i+1], data.indices[i+2]);
        edge e3(data.indices[i+2], data.indices[i]);

        edge inverse_e1(data.indices[i+1], data.indices[i]);
        edge inverse_e2(data.indices[i+2], data.indices[i+1]);
        edge inverse_e3(data.indices[i], data.indices[i+2]);

        if(edges.count(inverse_e1))
            edges.erase(inverse_e1);
        else
            edges.insert(e1);

        if(edges.count(inverse_e2))
            edges.erase(inverse_e2);
        else
            edges.insert(e2);

        if(edges.count(inverse_e3))
            edges.erase(inverse_e3);
        else
            edges.insert(e3);
    }
}

void sort_edge_tab(std::set<edge>& edges, std::vector<edge>& sort_edges)
{
    edge e1;
    std::set<edge> tempo(edges);
    unsigned int min = 999999;

    for(edge e : tempo)
    {
        if(e.first < min)
        {
            e1 = e;
            min = e.first;
        }
    }

    tempo.erase(e1);
    sort_edges.push_back(e1);
    edge del;
    for(int i = 0; i < edges.size(); i++)
    {
        for(edge e : tempo)
        {
            if(e.first == e1.second)
            {
                sort_edges.push_back(e);
                del = e1;
                e1 = e;
                break;
            }
        }
        tempo.erase(del);
    }
}

void place_on_circle(std::vector<edge> sort_edges,std::vector<Point>& My_circle)
{
    double pi = M_PI;
    int k = sort_edges.size();
    for(int i = 0; i < sort_edges.size(); i++)
    {
        double angle = (2*i*pi)/k;
        My_circle.push_back(Point(cos(angle), sin(angle), 0));
    }
}

void get_boundary_vertices(const std::vector<edge>& edges, std::set<int>& boundary_vertices)
{
    for(const edge& e : edges)
        boundary_vertices.insert(e.first);
}

void get_interior_vertices(const MeshIOData& data, const std::set<int>& boundary_vertices,
                           std::vector<int>& interior_vertices)
{
    for(int i = 0; i < data.positions.size(); i++)
        if(boundary_vertices.count(i) == 0)
            interior_vertices.push_back(i);
}

void vertices_to_mat_index(std::map<int,int>& vertex_to_var, std::vector<int> interior_vertices)
{
    int var_idx = 0;
    for(int ind : interior_vertices)
    {
        vertex_to_var[ind] = var_idx;
        var_idx++;
    }
}

void map_of_neighbors(const MeshIOData& data, std::map<int,std::set<int>>& neighbors,
                      std::set<int> boundary_vertices)
{
    for(int i = 0; i < data.indices.size(); i += 3)
    {
        int a = data.indices[i];
        int b = data.indices[i+1];
        int c = data.indices[i+2];
        neighbors[a].insert(b); neighbors[a].insert(c);
        neighbors[b].insert(a); neighbors[b].insert(c);
        neighbors[c].insert(a); neighbors[c].insert(b);
    }
}

void make_digged_matrice(Eigen::SparseMatrix<float>& M, const MeshIOData& data,
                         std::map<int,std::set<int>>& neighbors,
                         std::map<int,int>& vertex_to_var,
                         Eigen::VectorXf& rhs_u, Eigen::VectorXf& rhs_v,
                         std::set<int> boundary_vertices, std::vector<Point> mycircle)
{
    M = Eigen::SparseMatrix<float>(vertex_to_var.size(), vertex_to_var.size());
    std::vector<Eigen::Triplet<float>> triplets;
    rhs_u = Eigen::VectorXf(vertex_to_var.size());
    rhs_v = Eigen::VectorXf(vertex_to_var.size());
    rhs_u.setZero();
    rhs_v.setZero();

    for(auto it = vertex_to_var.begin(); it != vertex_to_var.end(); it++)
    {
        int vertex = it->first;
        int row    = it->second;
        int d      = neighbors[vertex].size();

        triplets.emplace_back(row, row, d);//degre du sommet  place a la diagonale 

        for(auto it2 = neighbors[vertex].begin(); it2 != neighbors[vertex].end(); it2++)
        {
            if(boundary_vertices.count(*it2))
            {
                rhs_u[row] += (1.0f) * mycircle[*it2].x;
                rhs_v[row] += (1.0f) * mycircle[*it2].y;
            }
            else
            {
                int col = vertex_to_var[*it2];
                triplets.emplace_back(row, col, -1.0f);
            }
        }
    }
    M.setFromTriplets(triplets.begin(), triplets.end());
}
void get_lscm_pined_vertices(
                              int& haut, int& bas,const MeshIOData& data )
{
    float max_y = -9999;
    float min_y =  9999;

    for(int i = 0; i < data.positions.size(); i++)
    {
        if(data.positions[i].y > max_y) {
            max_y = data.positions[i].y;
            haut = i;
        }
        if(data.positions[i].y < min_y) {
            min_y = data.positions[i].y;
            bas = i;
        }
    }
}

//on recupere tous les sommets qui sont ni le plus grand ni le plus petit
void get_lscm_interior_vertices(const MeshIOData& data,
                                 int haut, int bas,
                                 std::vector<int>& interior_vertices)
{
    for(int i = 0; i < data.positions.size(); i++)
        if(i != haut && i != bas)
            interior_vertices.push_back(i);
}

void vertices_to_mat_index_lscm(std::map<int,int>& vertex_to_var,
                                  const std::vector<int>& interior_vertices)
{
    int var_idx = 0;
    for(int idx : interior_vertices)
        vertex_to_var[idx] = var_idx++;
}
void make_LSM_matrice(const MeshIOData& data,
    const std::map<int,int>& vertex_to_var,
    int haut, int bas,
    Point Phaut, Point Pbas,
    Eigen::SparseMatrix<float>& system,
    Eigen::VectorXf& rhs)
    {
        float fsize=data.indices.size()/3;
        float vsize = data.indices.size();
        rhs = Eigen::VectorXf::Zero(2 * fsize);
        system   = Eigen::SparseMatrix<float>(2 * fsize, 2 * vsize -2);
        std::vector<Eigen::Triplet<float>> triplets_sys;
    std::vector<Eigen::Triplet<float>> triplets_rhs;
        int indices_faces[3];//on recuepre indice des faces
        Point points_faces[3];
        for(int i=0;i<fsize;i++)
        {
            for(int j=0;j<3;j++)
            {
                indices_faces[j]=data.indices[3*i+j];//pour la 1ere face ca donne bien 0,1,2 pour la 2eme 3*1+0=3 et 3*1+1=4 
            }
            for(int j=0;j<3;j++)
            {
                points_faces[j]=data.positions[indices_faces[j]];
            }
            Vector e0=normalize(points_faces[1]-points_faces[0]);
            Vector e2=cross(e0,points_faces[2]-points_faces[0]);
            Vector e1=normalize(cross(e2,e0));
        float dt = sqrt(length(cross(points_faces[1]-points_faces[0],points_faces[2]-points_faces[0])));   
        float ppx[3],ppy[3];
        Vector ppp0;
        float px[3],py[3];
        for(int j=0;j<3;j++)   
        {
                ppp0=points_faces[j]-points_faces[0];
                px[j]=dot(ppp0,e0);
                py[j]=dot(ppp0,e1);
        }  
        
        for(int j=0;j<3;j++)
        {
            float dax=py[(j+1)%3] - py[(j+2)%3];
          float  day = px[(j+2)%3] - px[(j+1)%3];
            dax=dax/dt;
            day=day/dt;
            int var=indices_faces[j] ;
            //variable_index(i)
            if(var==bas)
            {
                var=-1;
            }
            if(var==haut)
            {
                var=-2;
            }
            int res=var;
            if(var>bas)
            {
                res=res-1;
            }
            if(var>haut)
            {
                res=res-1;
            }
            if(var<0)
            {
            rhs[2*i,2*(-var-1)] = -day;
            rhs[2*i,2*(-var-1)+1] = -dax;
            rhs[2*i+1,2*(-var-1)] = dax;
            rhs[2*i+1,2*(-var-1)+1] = -day;
            }
            else
            {
                
 
                triplets_sys.emplace_back(2*i,     2*res,      day);
                triplets_sys.emplace_back(2*i,     2*res + 1,  dax);
                triplets_sys.emplace_back(2*i + 1, 2*res,     -dax);
                triplets_sys.emplace_back(2*i + 1, 2*res + 1,  day);
            }
        
        }   
    
        }
                system.setFromTriplets(triplets_sys.begin(), triplets_sys.end());



    }
bool init( )
{
    MeshIOData data;
    if(!read_meshio_data("./projets/parameterization-etu-synthese/assets/suzanne_uvsplit.obj", data))
        return false;

    count = data.indices.size();

    GLuint texture = read_texture(0, "./projets/parameterization-etu-synthese/assets/blender_checker.png");
    default_texture(0, texture);

    std::set<edge> edges;
    std::vector<edge> sort_edges;
    std::vector<Point> circle_positions;
    std::vector<Point> interior_positions;
    std::set<int> boundary_vertices;
    std::vector<Point> border_positions;
    std::vector<int> interior_vertices;
    std::map<int, std::set<int>> neighbors;
    std::map<int,int> vertex_to_var;

    init_edge_tab(edges, data);
    sort_edge_tab(edges, sort_edges);
    get_boundary_vertices(sort_edges, boundary_vertices);
    get_interior_vertices(data, boundary_vertices, interior_vertices);

    for(int idx : boundary_vertices)
        border_positions.push_back(data.positions[idx]);
    for(int idx : interior_vertices)
        interior_positions.push_back(data.positions[idx]);

    std::vector<Point> circle_coords(data.positions.size());
    place_on_circle(sort_edges, circle_positions);

    for(int i = 0; i < sort_edges.size(); i++)
        circle_coords[sort_edges[i].first] = circle_positions[i];

    map_of_neighbors(data, neighbors, boundary_vertices);
    vertices_to_mat_index(vertex_to_var, interior_vertices);

    Eigen::SparseMatrix<float> M;
    Eigen::VectorXf rhs_u, rhs_v;

    make_digged_matrice(M, data, neighbors, vertex_to_var, rhs_u, rhs_v, boundary_vertices, circle_coords);

    Eigen::ConjugateGradient<Eigen::SparseMatrix<float>> solver;
    solver.compute(M);
    Eigen::VectorXf sol_u = solver.solve(rhs_u);
    Eigen::VectorXf sol_v = solver.solve(rhs_v);

    border_vao     = create_buffers(circle_positions, {}, circle_positions, {});
    interior_vao   = create_buffers(interior_positions, {}, interior_positions, {});
    border_count   = circle_positions.size();
    interior_count = interior_positions.size();

    std::vector<Point> textcoords(data.positions.size());
    // j avais oublie de mettre les sommets des bords aussi 
    for(int i = 0; i < sort_edges.size(); i++)
        textcoords[sort_edges[i].first] = circle_positions[i];//l indice au bord recoit ces coordonnes
    for(auto it = vertex_to_var.begin(); it != vertex_to_var.end(); it++)
        textcoords[it->first] = Point(sol_u[it->second], sol_v[it->second], 0);

    vao      = create_buffers(data.positions, data.indices, textcoords, data.normals);
    vao_applat = create_buffers(textcoords, data.indices);

    std::cout << "edges: "      << edges.size()             << std::endl;
    std::cout << "sort_edges: " << sort_edges.size()        << std::endl;
    std::cout << "boundary: "   << boundary_vertices.size() << std::endl;
    std::cout << "interior: "   << interior_vertices.size() << std::endl;
    std::cout << "circle: "     << circle_positions.size()  << std::endl;
    //LSM ********************************************************************
    int haut = 0, bas = 0;
    Eigen::SparseMatrix<float> system;
Eigen::VectorXf rhs;
    get_lscm_pined_vertices(haut,bas,data);
    Point Phaut(0.5f,0.8f,0);
    Point Pbas(0.5f,0.2f,0);
    std::vector<int> interior_vertices_LSm;
    get_lscm_interior_vertices(data,haut,bas,interior_vertices_LSm);
    std::map<int,int> vertex_to_var_LSM ;
    vertices_to_mat_index_lscm(vertex_to_var_LSM,interior_vertices_LSm);
    make_LSM_matrice(data,vertex_to_var_LSM,haut,bas,Phaut,Pbas,system,rhs);
    Eigen::LeastSquaresConjugateGradient<Eigen::SparseMatrix<float>> solverLSM;
     solverLSM.compute(system);
    Eigen::VectorXf lscm_sol = solverLSM.solve(rhs);
 
    // reconstruction des coordonnees UV LSCM
    std::vector<Point> lscm_texcoords(data.positions.size());
    lscm_texcoords[haut] = Phaut;
    lscm_texcoords[bas]  = Pbas;
    for(auto it = vertex_to_var_LSM.begin(); it != vertex_to_var_LSM.end(); it++)
        lscm_texcoords[it->first] = Point(lscm_sol[2*it->second], lscm_sol[2*it->second+1], 0);
 
    vao_LSM        = create_buffers(data.positions, data.indices, lscm_texcoords, data.normals);
    vao_LSM_applat = create_buffers(lscm_texcoords, data.indices);
    return true;
}

void quit( )
{
    release_buffers(vao);
    release_buffers(vao_applat);
}

void draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Transform model;
    Transform view       = Translation(offset_x, offset_y, zoomFactor);
    Transform projection = Perspective(45, 1024.0 / 576.0, 0.1, 100);

    if(applat)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        if(!LSM)
        {
         draw(vao_applat, GL_TRIANGLES, count, model, view, projection);

        }
        else
        {
        draw(vao_LSM_applat, GL_TRIANGLES, count, model, view, projection);

        }
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        if(LSM)
        {
                    draw(vao_LSM, GL_TRIANGLES, count, model, view, projection);

        }
        else
        {
             draw(vao, GL_TRIANGLES, count, model, view, projection);

        }
    }

    glPointSize(6.0f);
}
int main(int argc, char** argv)
{
    Window window  = create_window(1024, 576);
    Context context = create_context(window);

    glViewport(0, 0, 1024, 576);
    glClearColor(0.2, 0.2, 0.2, 1);
    glClearDepth(1);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    if(!init())
        return 1;

    bool close = false;
    while(!close)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
                close = true;
            else if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                close = true;
            else if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)
                applat = !applat;
            
        }

        draw();
        SDL_GL_SwapWindow(window);
    }

    quit();
    release_context(context);
    release_window(window);
    return 0;
}