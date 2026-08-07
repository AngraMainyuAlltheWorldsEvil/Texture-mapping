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

#include "window.h"
#include <set>


GLuint vao= 0;
unsigned count= 0;

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

        // Quand on rencontre une arête a nouveau, elle est dans le sens inverse
        if(edges.count(inverse_e1))
        {
            edges.erase(e1);
        }
        else
        {
            edges.insert(e1);
        }

        if(edges.count(inverse_e2))
        {
            edges.erase(e2);
        }
        else
        {
            edges.insert(e2);
        }

        if(edges.count(inverse_e3))
        {
            edges.erase(e3);
        }
        else
        {
            edges.insert(e3);
        }
    }
}

void sort_edge_tab(std::set<edge>& edges, std::vector<edge>& sort_edges)
{
    
}

bool init( )
{
    MeshIOData data;
    if(!read_meshio_data("./projets/parameterization-etu-synthese/assets/suzanne_uvsplit.obj", data))
        return false;   // erreur de lecture
    
    vao= create_buffers(data.positions, data.indices, data.positions, data.normals);
    /* ou 
        vao= create_buffers(data.positions, data.indices, data.texcoords, data.normals);
        mais s'il y a des coordonn�es de textures dans l'objet, il faut aussi une texture pour le dessiner.
        on verra plus tard. donc pour l'instant on ne cree pas de coordonn�es de texture...
     */ 
     
    count = data.indices.size();

    GLuint texture= read_texture(0, "./projets/parameterization-etu-synthese/assets/blender_checker.png");
    default_texture(0, texture);

    std::set<edge> edges;
    std::vector<edge> sort_edges;
    init_edge_tab(edges, data);
    sort_edge_tab(edges, sort_edges);
    
    return true;
}

void quit( ) 
{
    release_buffers(vao);
}

void draw( )
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    Transform model;    // placer le modele
    Transform view= Translation(0, -2, -10);     // camera
    Transform projection = Perspective(45, 1024.0 / 576.0, 0.1, 100);
    
    draw(vao, GL_TRIANGLES, count, model, view, projection);
}


int main(int argc, char** argv)
{
    Window window= create_window(1024, 576);
    Context context= create_context(window);    // a virer
    
    // etat openGL de base / par defaut
    glViewport(0, 0, 1024, 576);
    glClearColor(0.2, 0.2, 0.2, 1);
    glClearDepth(1);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    
    if(!init())
        return 1;
    
    bool close= false;
    while(!close)
    {
        SDL_Event event;
        
        // recuperer un evenement a la fois, poll event renvoie faux lorsqu'ils ont tous ete traite
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
                close= true;  // sortir si click sur le bouton 'fermer' de la fenetre
            else if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                close= true;  // sortir si la touche esc / echapp est enfoncee
        }
        
        // dessiner
        draw();
        
        // presenter / montrer le resultat
        SDL_GL_SwapWindow(window);
    }
    
    quit();
    
    release_context(context);
    release_window(window);
    return 0;
}
