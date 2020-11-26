
//header
#include "engine/3d/renderer/renderer_simple.hpp"

//other library files
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <GL/glew.h>

//your project files
#include "engine/resources/resource_manager.hpp"
#include "engine/graphics/render_command.hpp"

namespace fightingengine {

RendererSimple::RendererSimple(RandomState& rnd, int width, int height)
{
    //Configure OpenGL
    RenderCommand::init();
    //RenderCommand::set_clear_colour(glm::vec4(0.0, 0.482f, 0.655f, 1.0));
    RenderCommand::set_clear_colour( { 0.6f, 0.6f, 0.6f, 1.0f } );

    width_ = width;
    height_ = height;

    //load shaders
    ResourceManager::load_shader("assets/shaders/blinn-phong/", {"lit.vert", "lit_directional.frag"}, "lit");
    ResourceManager::load_shader("assets/shaders/pbr", {"pbr.vert", "pbr.frag"}, "pbr");

    //shaders & meshes 
    flat_shader_ = ResourceManager::get_shader("lit");
    pbr_shader_ = ResourceManager::get_shader("pbr");

    cube = std::make_shared<primitives::Cube>();  
    plane = std::make_shared<primitives::Plane>(1, 1);  

    //textures
    ResourceManager::load_texture( "assets/textures/pbr_metalgrid/metalgrid2_basecolor.png", "albedo" );
    ResourceManager::load_texture( "assets/textures/pbr_metalgrid/metalgrid2_normal-dx.png", "normal" );
    ResourceManager::load_texture( "assets/textures/pbr_metalgrid/metalgrid2_metallic.png", "metallic" );
    ResourceManager::load_texture( "assets/textures/pbr_metalgrid/metalgrid2_roughness.png", "roughness" );
    ResourceManager::load_texture( "assets/textures/pbr_metalgrid/metalgrid2_AO.png", "ao" );

    //TODO(Turtwiggy) finish model loading
    //object_ = ResourceManager::load_model("assets/models/lizard_wizard/lizard_wizard->obj", "Object");

    //skybox
    TextureCube cubemap = ResourceManager::load_texture_cube("assets/skybox/skybox-default/", "default-skybox");
    cubemap_ = std::make_shared<TextureCube>(cubemap);
    background = new Background();
    background->Material->set_texture_cube("DefaultCubemap", cubemap_.get());
}


// renders (and builds at first invocation) a sphere
// -------------------------------------------------

void RendererSimple::renderSphere()
{
    draw_calls_ += 1;

    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359;
        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = indices.size();

        std::vector<float> data;
        for (std::size_t i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        float stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

void RendererSimple::update(const Application& app, float delta_time, FlyCamera& camera, RandomState& rnd, const std::vector<glm::vec3>& cube_pos)
{
    camera.Update(delta_time);
    glm::mat4 view_projection =  camera.get_view_projection_matrix();

    draw_calls_ = 0;

    // render pass: rendering phase
    // ---------------------------
    RenderCommand::clear();

    // flat_shader_.bind();
    // flat_shader_.set_mat4("view_projection", view_projection);
    // //flat_shader_.set_vec3("viewPos", camera.Position);

    // flat_shader_.set_vec3("light.direction", -0.2f, -1.0f, -0.3f);

    // glm::vec3 light_diffuse = light_colours[0]   * glm::vec3(0.5f); // decrease the influence
    // glm::vec3 light_ambient = light_diffuse * glm::vec3(0.2f); // low influence
    // flat_shader_.set_vec3("light.ambient", light_ambient);
    // flat_shader_.set_vec3("light.diffuse", light_diffuse);
    // flat_shader_.set_vec3("light.specular", 1.0f, 1.0f, 1.0f);

    // // Cube: Material
    // flat_shader_.set_vec3("material.ambient", glm::vec3(1.0f, 0.0f, 0.0f));
    // flat_shader_.set_vec3("material.diffuse", glm::vec3(1.0f, 0.0f, 0.0f));
    // flat_shader_.set_vec3("material.specular", 0.5f, 0.5f, 0.5f);
    // flat_shader_.set_float("material.shininess", 32.0f);
    // // Cube: Draw
    // for(int i = 0; i < cube_pos.size(); i++)
    // {
    //     // calculate the model matrix for each object and pass it to shader before drawing
    //     glm::mat4 model = glm::mat4(1.0f);
    //     model = glm::translate(model, cube_pos[i]);
    //     model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    //     flat_shader_.set_mat4("model", model);

    //     render_mesh(cube);
    // }    

    // //Plane: Scale
    // glm::mat4 model = glm::mat4(1.0f);
    // model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    // model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));
    // model = glm::rotate(model, glm::radians(90.0f), glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f)));
    // flat_shader_.set_mat4("model", model);
    // //Plane: Material
    // flat_shader_.set_vec3("material.ambient", glm::vec3(0.0f, 1.0f, 0.0f));
    // flat_shader_.set_vec3("material.diffuse", glm::vec3(0.0f, 1.0f, 0.0f));
    // flat_shader_.set_vec3("material.specular", 0.5f, 0.5f, 0.5f);
    // flat_shader_.set_float("material.shininess", 32.0f);
    // render_mesh(plane);



    // some pbr stuff 
    // --------------

    glm::vec3 light_positions[] = {
        glm::vec3(-10.0f,  10.0f, 10.0f),
        glm::vec3( 10.0f,  10.0f, 10.0f),
        glm::vec3(-10.0f, -10.0f, 10.0f),
        glm::vec3( 10.0f, -10.0f, 10.0f),
    };
    glm::vec3 light_colours[] = {
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f)
    };
    int nrRows = 7;
    int nrColumns = 7;
    float spacing = 2.5f;
    
    pbr_shader_.bind();
    pbr_shader_.set_mat4("view_projection", view_projection);  
    pbr_shader_.set_vec3("camPos", camera.Position);  
    pbr_shader_.set_vec3("albedo", 0.5f, 0.0f, 0.0f);
    pbr_shader_.set_float("ao", 1.0f);

    // Material	                F0 (Linear)	            F0 (sRGB)
    // Water	                (0.02, 0.02, 0.02)	    (0.15, 0.15, 0.15)  	
    // Plastic / Glass (Low)	(0.03, 0.03, 0.03)	    (0.21, 0.21, 0.21)	
    // Plastic High	            (0.05, 0.05, 0.05)	    (0.24, 0.24, 0.24)	
    // Glass (high) / Ruby	    (0.08, 0.08, 0.08)	    (0.31, 0.31, 0.31)	
    // Diamond	                (0.17, 0.17, 0.17)	    (0.45, 0.45, 0.45)	
    // Iron	                    (0.56, 0.57, 0.58)	    (0.77, 0.78, 0.78)	
    // Copper	                (0.95, 0.64, 0.54)  	(0.98, 0.82, 0.76)	
    // Gold	                    (1.00, 0.71, 0.29)  	(1.00, 0.86, 0.57)	
    // Aluminium	            (0.91, 0.92, 0.92)  	(0.96, 0.96, 0.97)	
    // Silver	                (0.95, 0.93, 0.88)  	(0.98, 0.97, 0.95

    // ResourceManager::get_texture("albedo")->Bind(0);
    // ResourceManager::get_texture("normal")->Bind(1);
    // ResourceManager::get_texture("metallic")->Bind(2);
    // ResourceManager::get_texture("roughness")->Bind(3);
    // ResourceManager::get_texture("ao")->Bind(4);

    // render rows*column number of spheres with material properties 
    // defined by textures (they all have the same material properties)
    glm::mat4 model = glm::mat4(1.0f);
    for (int row = 0; row < nrRows; ++row)
    {
        pbr_shader_.set_float("metallic", (float)row / (float)nrRows);

        for (int col = 0; col < nrColumns; ++col)
        {
            // we clamp the roughness to 0.025 - 1.0 as perfectly smooth
            // surfaces (roughness of 0.0) tend to look a bit off
            // on direct lighting.
            pbr_shader_.set_float("roughness", glm::clamp((float)col / (float)nrColumns, 0.05f, 1.0f));

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(
                (float)(col - (nrColumns / 2)) * spacing,
                (float)(row - (nrRows / 2)) * spacing,
                0.0f
            ));
            pbr_shader_.set_mat4("model", model);

            renderSphere();
        }
    }

    // render light source (simply re-render sphere at light positions)
    // this looks a bit off as we use the same shader, but it'll make their positions obvious and 
    // keeps the codeprint small.
    float time_since_launch = app.get_time_since_launch();
    for (unsigned int i = 0; i < sizeof(light_positions) / sizeof(light_positions[0]); ++i)
    {
        glm::vec3 newPos = light_positions[i] + glm::vec3(sin(time_since_launch * 5.0) * 5.0, 0.0, 0.0);
        newPos = light_positions[i];
        pbr_shader_.set_vec3("lightPositions[" + std::to_string(i) + "]", newPos);
        pbr_shader_.set_vec3("lightColors[" + std::to_string(i) + "]", light_colours[i]);

        model = glm::mat4(1.0f);
        model = glm::translate(model, newPos);
        model = glm::scale(model, glm::vec3(0.5f));
        pbr_shader_.set_mat4("model", model);
        renderSphere();
    }

    //draw_skybox(view_projection);
    //flat_shader_.unbind();
}

void RendererSimple::draw_skybox(const glm::mat4& view_projection)
{
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(1000.0f, 1000.0f, 1000.0f));

    glDepthFunc(background->Material->DepthCompare); 

    background->Material->get_shader()->bind();
    background->Material->get_shader()->set_mat4("view_projection", view_projection);
    background->Material->get_shader()->set_mat4("model", model);

    // skybox cube
    auto* samplers = background->Material->get_sampler_uniforms();
    for (auto it = samplers->begin(); it != samplers->end(); ++it)
    {
        if (it->second.Type == SHADER_TYPE::SHADER_TYPE_SAMPLERCUBE)
            it->second.TextureCube->Bind(it->second.Unit);
        else
            it->second.Texture->Bind(it->second.Unit);
    }
    
    // , background->Material->get_shader()
    render_mesh(background->Mesh);

    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
}

int RendererSimple::get_draw_calls() { return draw_calls_; }

} //namespace fightingengine

//NOTE: this file contains blur and bloom effects

// std::array<unsigned int, 2> Renderer::hdr_colour_buffer(int width, int height, unsigned int hdr_fbo)
// {
//    // create 2 floating point color buffers (1 for normal rendering, other for brightness treshold values)
//    unsigned int colorBuffers[2];
//    glGenTextures(2, colorBuffers);
//    for (unsigned int i = 0; i < 2; i++)
//    {
//        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
//        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//        // attach texture to framebuffer
//        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
//    }
//    // create and attach depth buffer (renderbuffer)
//    unsigned int rboDepth;
//    glGenRenderbuffers(1, &rboDepth);
//    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
//    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

//    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
//    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
//    glDrawBuffers(2, attachments);

//    // finally check if framebuffer is complete
//    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
//        std::cout << "Framebuffer not complete!" << std::endl;
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);

//    std::array<unsigned int, 2> color_buffer_array;
//    color_buffer_array[0] = colorBuffers[0];
//    color_buffer_array[1] = colorBuffers[1];
//    return color_buffer_array;
// }

// void hdr_bloom_init()
// {
   //// configure (floating point) framebuffers
   //// ---------------------------------------
   //s_Data->hdr_fbo = create_fbo();
   //s_Data->hdr_colour_buffers = hdr_colour_buffer(screen_width, screen_height, s_Data->hdr_fbo);

   //// ping-pong-framebuffer for blurring
   //unsigned int pingpongFBO[2];
   //unsigned int pingpongColorbuffers[2];
   //glGenFramebuffers(2, pingpongFBO);
   //glGenTextures(2, pingpongColorbuffers);
   //for (unsigned int i = 0; i < 2; i++)
   //{
   //    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
   //    glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
   //    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_width, screen_height, 0, GL_RGBA, GL_FLOAT, NULL);
   //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
   //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   //    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
   //    // also check if framebuffers are complete (no need for depth buffer)
   //    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
   //        std::cout << "Framebuffer not complete!" << std::endl;
   //}
   //s_Data->pingpong_fbo[0] = pingpongFBO[0];
   //s_Data->pingpong_fbo[1] = pingpongFBO[1];
   //s_Data->pingpong_colour_buffers[0] = pingpongColorbuffers[0];
   //s_Data->pingpong_colour_buffers[1] = pingpongColorbuffers[1];

   //// lighting info
   //// -------------
   //// positions
   //std::vector<glm::vec3> light_positions;
   //light_positions->push_back(glm::vec3(0.0f, 0.5f, 1.5f));
   //light_positions->push_back(glm::vec3(-4.0f, 0.5f, -3.0f));
   //light_positions->push_back(glm::vec3(3.0f, 0.5f, 1.0f));
   //light_positions->push_back(glm::vec3(-->8f, 2->4f, -1.0f));
   //s_Data->light_positions = light_positions;
   //// colors
   //std::vector<glm::vec3> lightColors;
   //lightColors->push_back(glm::vec3(5.0f, 5.0f, 5.0f));
   //lightColors->push_back(glm::vec3(10.0f, 0.0f, 0.0f));
   //lightColors->push_back(glm::vec3(0.0f, 0.0f, 15.0f));
   //lightColors->push_back(glm::vec3(0.0f, 5.0f, 0.0f));
   //s_Data->light_colours = lightColors;

   //// shader configuration
   //// --------------------
   //s_Data->object_shader->use();
   //s_Data->object_shader->setInt("texture_diffuse1", 0);
   //s_Data->blur_shader->use();
   //s_Data->blur_shader->setInt("image", 0);
   //s_Data->hdr_bloom_final_shader->use();
   //s_Data->hdr_bloom_final_shader->setInt("scene", 0);
   //s_Data->hdr_bloom_final_shader->setInt("bloomBlur", 1);
// }

// void hdr_bloom_draw()
// {
   //   // 1-> render scene into floating point framebuffer
   //// -----------------------------------------------
   //   glBindFramebuffer(GL_FRAMEBUFFER, s_Data->hdr_fbo);
   //   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   //   s_Data->object_shader->use();
   //   s_Data->object_shader->setMat4("view_projection", view_projection);
   //   s_Data->object_shader->setVec3("viewPos", desc->camera->Position);
   //   glActiveTexture(GL_TEXTURE0);
   //   glBindTexture(GL_TEXTURE_2D, s_Data->wood_texture);
   //   // set lighting uniforms
   //   for (unsigned int i = 0; i < s_Data->light_positions->size(); i++)
   //   {
   //       s_Data->object_shader->setVec3("lights[" + std::to_string(i) + "]->Position", s_Data->light_positions[i]);
   //       s_Data->object_shader->setVec3("lights[" + std::to_string(i) + "]->Color", s_Data->light_colours[i]);
   //   }

   //   // create one large cube that acts as the floor
   //   model = glm::mat4(1.0f);
   //   model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0->0));
   //   model = glm::scale(model, glm::vec3(12.5f, 0.5f, 12.5f));
   //   s_Data->object_shader->setMat4("model", model);
   //   renderCube(s_Data->stats->DrawCalls);

   //   // then create multiple cubes as the scenery
   //   glBindTexture(GL_TEXTURE_2D, s_Data->second_texture);

   //   model = glm::mat4(1.0f);
   //   model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0->0));
   //   model = glm::scale(model, glm::vec3(0.5f));
   //   s_Data->object_shader->setMat4("model", model);
   //   //state->cubes[0]->model->draw(s_Data->object_shader, s_Data->stats->DrawCalls);
   //   renderCube(s_Data->stats->DrawCalls);
   //   model = glm::mat4(1.0f);
   //   model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1->0));
   //   model = glm::scale(model, glm::vec3(0.5f));
   //   s_Data->object_shader->setMat4("model", model);
   //   renderCube(s_Data->stats->DrawCalls);
   //   model = glm::mat4(1.0f);
   //   model = glm::translate(model, glm::vec3(-1.0f, -1.0f, 2->0));
   //   model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1->0, 0->0, 1->0)));
   //   s_Data->object_shader->setMat4("model", model);
   //   renderCube(s_Data->stats->DrawCalls);
   //   model = glm::mat4(1.0f);
   //   model = glm::translate(model, glm::vec3(0.0f, 2->7f, 4->0));
   //   model = glm::rotate(model, glm::radians(23.0f), glm::normalize(glm::vec3(1->0, 0->0, 1->0)));
   //   model = glm::scale(model, glm::vec3(1->25));
   //   s_Data->object_shader->setMat4("model", model);
   //   renderCube(s_Data->stats->DrawCalls);
   //   model = glm::mat4(1.0f);
   //   model = glm::translate(model, glm::vec3(-2.0f, 1.0f, -3->0));
   //   model = glm::rotate(model, glm::radians(124.0f), glm::normalize(glm::vec3(1->0, 0->0, 1->0)));
   //   s_Data->object_shader->setMat4("model", model);
   //   renderCube(s_Data->stats->DrawCalls);
   //   model = glm::mat4(1.0f);
   //   model = glm::translate(model, glm::vec3(-3.0f, 0.0f, 0->0));
   //   model = glm::scale(model, glm::vec3(0.5f));
   //   s_Data->object_shader->setMat4("model", model);
   //   renderCube(s_Data->stats->DrawCalls);



   //   // show all the light sources as bright cubes
   //   s_Data->light_positions[0] = state->cubes[0]->transform->Position;

   //   s_Data->light_shader->use();
   //   s_Data->light_shader->setMat4("view_projection", view_projection);
   //   for (unsigned int i = 0; i < s_Data->light_positions->size(); i++)
   //   {
   //       model = glm::mat4(1.0f);
   //       model = glm::translate(model, glm::vec3(s_Data->light_positions[i]));
   //       model = glm::scale(model, glm::vec3(0->25f));
   //       s_Data->light_shader->setMat4("model", model);
   //       s_Data->light_shader->setVec3("lightColor", s_Data->light_colours[i]);
   //       renderCube(s_Data->stats->DrawCalls);
   //   }
   //   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   //   // 2-> blur bright fragments with two-pass Gaussian Blur 
   //   // --------------------------------------------------
   //   bool horizontal = true, first_iteration = true;
   //   unsigned int amount = 10;
   //   s_Data->blur_shader->use();
   //   s_Data->blur_shader->setInt("blur_x", amount / 2);
   //   s_Data->blur_shader->setInt("blur_y", amount / 2);
   //   for (unsigned int i = 0; i < amount; i++)
   //   {
   //       glBindFramebuffer(GL_FRAMEBUFFER, s_Data->pingpong_fbo[horizontal]);
   //       s_Data->blur_shader->setInt("horizontal", horizontal);
   //       glBindTexture(GL_TEXTURE_2D, first_iteration ? s_Data->hdr_colour_buffers[1] : s_Data->pingpong_colour_buffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
   //       renderQuad();
   //       horizontal = !horizontal;
   //       if (first_iteration)
   //           first_iteration = false;
   //   }
   //   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   //   // 3-> now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
   //   // --------------------------------------------------------------------------------------------------------------------------
   //   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   //   s_Data->hdr_bloom_final_shader->use();
   //   glActiveTexture(GL_TEXTURE0);
   //   glBindTexture(GL_TEXTURE_2D, s_Data->hdr_colour_buffers[0]);
   //   glActiveTexture(GL_TEXTURE1);
   //   glBindTexture(GL_TEXTURE_2D, s_Data->pingpong_colour_buffers[!horizontal]);
   //   s_Data->hdr_bloom_final_shader->setInt("bloom", desc->hdr);
   //   s_Data->hdr_bloom_final_shader->setFloat("exposure", desc->exposure);
   //   renderQuad();
   //   //std::cout << "bloom: " << (desc->hdr ? "on" : "off") << "| exposure: " << desc->exposure << std::endl;
// }

