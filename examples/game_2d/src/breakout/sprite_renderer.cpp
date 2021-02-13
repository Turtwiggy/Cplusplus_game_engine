
// header
#include "breakout/sprite_renderer.hpp"

// other project headers
#include <GL/glew.h>

// engine project headers
#include "engine/opengl/texture.hpp"

namespace game2d {

namespace sprite_renderer {

unsigned int quadVAO = 0;
void
render_quad()
{
  if (quadVAO == 0) {
    float vertices[] = {
      // clang-format off
        // pos      // tex
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 

        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f
      // clang-format on
    };
    unsigned int VBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(quadVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }
  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}

void
draw_sprite(Shader& shader, GameObject& game_object)
{
  draw_sprite(shader, game_object.tex_slot, game_object.transform);
}

void
draw_sprite(Shader& shader, int tex_slot, Transform& t)
{
  draw_sprite(shader, tex_slot, t.position, t.scale, t.angle, t.colour);
}

void
draw_sprite(Shader& shader, int tex_slot, glm::vec2 position, glm::vec2 size, float angle, glm::vec3 color)
{
  shader.bind();

  glm::mat4 model = glm::mat4(1.0f);

  model = glm::translate(model, glm::vec3(position, 0.0f)); // move origin of rotation to center of quad

  model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f));
  model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));   // then rotate
  model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // move origin back

  // then scale
  model = glm::scale(model, glm::vec3(size, 1.0f));

  shader.set_mat4("model", model);
  shader.set_vec4("sprite_colour", glm::vec4{ color.x, color.y, color.z, 1.0f });
  // shader.set_int("tex", tex_slot);

  // bind_tex(tex_slot);

  render_quad();

  // unbind_tex();
};

} // namespace sprite_renderer

} // namespace game2d