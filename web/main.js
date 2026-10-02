import { WebGPURenderer } from "https://esm.sh/three@0.185.1/webgpu.js";
import * as THREE from "https://esm.sh/three@0.185.1";

export let Scene = null;
export let Camera = null;
export let Renderer = null;
export let Canvas = null;

export let Stud = null;
export let StudScale = 4;

export let Players = [];

function Deg(Degrees) {
  return Degrees * (Math.PI / 180);
}

export async function EngineLoad(WebGpu = false) {
  World = new CANNON.World({
    gravity: new CANNON.Vec3(0, -196.2, 0)
  });
  World.defaultContactMaterial.friction = 0;
  World.solver.iterations = 10;
  World.broadphase = new CANNON.NaiveBroadphase();

  Scene = new THREE.Scene();

  Camera = new THREE.PerspectiveCamera(
    75,
    Width / Height,
    0.1,
    1000
  );

  console.log("[folk] loading: renderer (webgpu=" + WebGpu + ")");

  if (WebGpu) {
    console.log("[folk] loading: webgpu");

    Renderer = new WebGPURenderer({
      antialias: true
    });

    if (!navigator.gpu) {
      Renderer = new THREE.WebGLRenderer({
        antialias: false
      });
    } else {
      await Renderer.init();
    }
  } else {
    console.log("[folk] loading: webgl");

    Renderer = new THREE.WebGLRenderer({
      antialias: false
    });
  }

  Canvas = Renderer.domElement;

  Renderer.setSize(Width, Height);
  Renderer.setClearColor(0x7cc6e7);
  Renderer.setPixelRatio(1);
  Renderer.shadowMap.enabled = true;
  Renderer.shadowMap.type = THREE.PCFShadowMap;

  document.body.appendChild(Canvas);

  Csm = new CSM({
    maxFar: 500,
    cascades: 3,
    mode: "uniform",
    parent: Scene,
    shadowMapSize: 1024,
    lightDirection: new THREE.Vector3(-1, -1, -1),
    camera: Camera
  });

  for (const Light of Csm.lights) {
    Light.intensity = 4;
  }

  const AmbientLight = new THREE.AmbientLight(0xffffff, 2);
  Scene.add(AmbientLight);

  Stud = new THREE.TextureLoader().load(
    "/api/images/stud.png",
    (Texture) => {
      Texture.wrapS = THREE.RepeatWrapping;
      Texture.wrapT = THREE.RepeatWrapping;
      Texture.magFilter = THREE.LinearFilter;
      Texture.minFilter = THREE.LinearMipmapLinearFilter;
    }
  );

  document.addEventListener("mousemove", (Event) => {
  });

  document.addEventListener("mousedown", (Event) => {
    MouseDown[Event.button] = true;
  });

  document.addEventListener("mouseup", (Event) => {
    MouseDown[Event.button] = false;
  });

  document.addEventListener("keydown", (Event) => {
    const Code = Event.code;
  });

  document.addEventListener("keyup", (Event) => {
    if (document.activeElement === ChatInput) return;

    let Code = Event.code;

    if (Event.code == "ArrowUp") {
      Code = "KeyW";
    } else if (Event.code == "ArrowDown") {
      Code = "KeyS";
    }

    KeyDown[Code] = false;
  });

  document.addEventListener("contextmenu", (Event) => {
    Event.preventDefault();
  });

  window.addEventListener("resize", () => {
    Camera.aspect = window.innerWidth / window.innerHeight;
    Camera.updateProjectionMatrix();
    Renderer.setSize(window.innerWidth, window.innerHeight);
  });
}

export async function EngineLoadGame() {
  Scene.traverse((Object) => {
    if (Object.isMesh) {
      Scene.remove(Object);
    }
  });

  const GameResponse = await fetch("game.json");
  const GameData = await GameResponse.json();

  await GameLoad(GameData);
}

export function EngineHandleInput(Dt) {
}

export function EngineMove(Object, Target) {
  Object.position.copy(Target);
}

export function EngineRotate(Object, Target) {
  const Quaternion = new THREE.Quaternion().setFromEuler(
    new THREE.Euler(
      Target.x,
      Target.y,
      Target.z
    )
  );

  Object.quaternion.copy(Quaternion);
}

export function EngineLoop() {
  requestAnimationFrame(EngineLoop);

  Scene.updateMatrixWorld();
  Renderer.render(Scene, Camera);

  ui_draw();
  window.frame_callback();
}
