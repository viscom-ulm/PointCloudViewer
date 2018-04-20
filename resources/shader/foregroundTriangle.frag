#version 330 core

in vec3 fragColor;
out vec4 color;

void main()
{
    //color = vec4(fragColor, 1.0);
   
    

    if(fragColor.x >= 0 && fragColor.x <= 1 && fragColor.y >= 0 && fragColor.y <= 1) {
        vec2 pos = mod(fragColor.xy*1000*4.5,vec2(1000));
        if ((pos.x > 50.0) && (pos.y > 50.0)){
        color=vec4(0.0);
        }
        if ((pos.x < 50.0) && (pos.y < 50.0)){
        color=vec4(0.0);
        }
        if ((pos.x < 50.0) && (pos.y > 50.0)){
        color=vec4(10.0);
        }
        if ((pos.x > 50.0) && (pos.y < 50.0)){
        color=vec4(1.0);
        }
    }

    
}