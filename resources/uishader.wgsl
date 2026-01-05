struct VertexInput {
  @location(0) position: vec2f,
  @location(1) uv: vec2f,
  @location(2) color: vec4f,
};

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) color: vec4f,
  @location(1) uv: vec2f,
};

struct UIUniforms {
  projectionMatrix: mat4x4f,
  transformMatrix: mat4x4f,
  color: vec4f,
  time: f32,        
};

@group(0) @binding(0) var<uniform> u: UIUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  var out: VertexOutput;
  out.position = u.projectionMatrix * u.transformMatrix * vec4f(in.position, 0.0, 1.0);
  out.color = in.color;
  out.uv = in.uv;
  return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  return u.color;
}
