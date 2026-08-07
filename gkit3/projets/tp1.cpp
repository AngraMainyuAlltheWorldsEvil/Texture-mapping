
#include "image.h"
#include "image_io.h"


int main( )
{
    Image image(1024, 512);
    
    // exemple : parcours tous les pixels de l'image
    for(unsigned i= 0; i < image.size(); i++)
        image(i)= Color(0.5);
    
    // exemple : parcours tous les pixels de l'image
    for(int py= 0; py < image.height(); py++)
    for(int px= 0; px < image.width(); px++)
        // damier 8x8...
        if((px & 8) ^ (py & 8))
            image(px, py)= Color(1);
    
    // enregistre l'image, de plusieurs manieres...
    write_image_png(image, "image.png");
    write_image_bmp(image, "image.bmp");
    write_image_hdr(image, "image.hdr");
    
    return 0;
}
