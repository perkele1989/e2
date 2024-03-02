
layout(std140) uniform entity 
{
    mat4 modelMatrix;
}

#if defined(SM_Skinned)
layout(std140) uniform skin 
{
     mat4 skin[64];
}
#endif