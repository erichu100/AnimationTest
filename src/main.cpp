#include "Engine.hpp"
#include "Graphics.hpp"
#include "UI.hpp"
#include "Transform.hpp"
#include "Controller.hpp"
#include "Physics.hpp"
#include "Time.hpp"
#include "Log.hpp"
#include "Audio.hpp"
#include "Input.hpp"
#include "GameState.hpp"
#include "Debug.hpp"
#include <iostream>

using Aspen::Engine::Engine;
using Aspen::Time::Time;
using Aspen::GameState::GameState;
using Aspen::GameState::GameStateManager;
using Aspen::Graphics::Graphics;
using Aspen::Object::Object;

void ChangeScene(Aspen::Graphics::UI::Button *button, std::string scene, GameStateManager *gsm)
{
  gsm->SetCurrentState(scene);
}

class Wall : public Aspen::Graphics::Rectangle
{
  
  public:
  Wall(Object *parent = nullptr, std::string name = "Wall") : Aspen::Graphics::Rectangle (SDL_Rect({0, 0, 32, 32}), Aspen::Graphics::Colors::BLACK, true, parent, name){
    CreateChild<Aspen::Physics::AABBCollider>()->SetSize(32, 32);
    GetTransform()->SetPosition(200,200);
  }
  void OnUpdate(){
  }

};
class Player1 : public Aspen::Object::Object
{
  //Constants
  const int startX = 324;
  const int fps = 60;
  const float dashLength = 0.3f;
  const float dashFrames = dashLength*60;
  const int dashMultiplier = 2;
  const int runxVelocity = 5;
  const float gravity = 0.25f;
  const int jumpStrength = -25;
  const float rollDistance = dashLength*runxVelocity*dashMultiplier;
  const float rollSpeed = runxVelocity*dashMultiplier;
  const float rollRotationPerFrame = 2*M_PI/fps;
  
  //Changing constants
  bool rightFacing = true;
  bool dash = false;
  double timer = 0;
  double startDash = 0;
  int rollFrame = 0;
  int blockFrame = 0;
  int guardBreakFrame = 0;
  int shootFrame = 0;
  int attackFrame = 0;

  //Gamestates
  bool rolling = false;
  bool blocking = false;
  bool blocked = false;
  bool guardBreaking = false;
  bool shooting = false;
  bool jumping = false;
  bool running = false;
  bool attacking = false;

  Aspen::Graphics::Animation *Idle;
  Aspen::Graphics::Animation *Running;
  Aspen::Graphics::Animation *Blocking;
  Aspen::Graphics::Animation *Blocked;
  Aspen::Graphics::Animation *Attacking;
  Aspen::Graphics::Animation *Shooting;
  Aspen::Graphics::Animation *Guardbreak;
  Aspen::Graphics::Animation *Rolling;
  Aspen::Graphics::Animation *CurrentAnimation;

  public:
    Player1(Object *parent = nullptr, std::string name = "player1") : Aspen::Object::Object(parent, name) {
      Idle = new Aspen::Graphics::Animation(new Aspen::Graphics::UniformSpritesheet("./resources/Idle.png", 200, 200, 2, nullptr, "player1Idle"), 1.0f/2.0f, this, "player1");
      Running = new Aspen::Graphics::Animation(new Aspen::Graphics::UniformSpritesheet("./resources/Run.png", 200, 200, 1, nullptr, "player1Running"), 1, this, "player1");
      Rolling = new Aspen::Graphics::Animation(new Aspen::Graphics::UniformSpritesheet("./resources/Roll.png", 200, 200, 1, nullptr, "player1Running"), 1, this, "player1");
      Blocking = new Aspen::Graphics::Animation(new Aspen::Graphics::UniformSpritesheet("./resources/Block.png", 200, 200, 3, nullptr, "player1Blocking"), 1.0f/12.0f, this, "player1");
      Blocked = new Aspen::Graphics::Animation(new Aspen::Graphics::UniformSpritesheet("./resources/Blocked.png", 200, 200, 1, nullptr, "player1Blocked"), 1.0f, this, "player1");
      Attacking = new Aspen::Graphics::Animation(new Aspen::Graphics::UniformSpritesheet("./resources/Attacc.png", 200, 200, 4, nullptr, "player1Attacking"), 1.0f/12.0f, this, "player1");
      Shooting = new Aspen::Graphics::Animation(new Aspen::Graphics::UniformSpritesheet("./resources/G U N.png", 200, 200, 10, nullptr, "player1SpriteSheet"), 1.0f/12.0f, this, "player1");
      Guardbreak = new Aspen::Graphics::Animation(new Aspen::Graphics::UniformSpritesheet("./resources/GuardBreak.png", 200, 200, 8, nullptr, "player1SpriteSheet"), 1.0f/12.0f, this, "player1");
      
      AddChild(Rolling);
      Rolling->Deactivate();

      AddChild(Blocking);
      Blocking->Deactivate();

      AddChild(Blocked);
      Blocked->Deactivate();

      AddChild(Guardbreak);
      Guardbreak->Deactivate();

      AddChild(Shooting);
      Shooting->Deactivate();

      AddChild(Running);
      Running->Deactivate();

      AddChild(Attacking);
      Attacking->Deactivate();

      AddChild(Idle);
      Idle->Deactivate();

      CurrentAnimation = Idle;
      CurrentAnimation->Activate();
      


      CreateChild<Aspen::Transform::Transform>();
      GetTransform()->SetPosition(100,223);
      CreateChild<Aspen::Physics::Rigidbody>();
      CreateChild<Aspen::Physics::AABBCollider>()->SetSize(80, 80);
    }
    void OnUpdate(){

      //Set velocities to current velocities
      float xVelocity = GetRigidbody()->GetVelocityX();
      float yVelocity = GetRigidbody()->GetVelocityY();
      timer += FindAncestorOfType<Engine>()->FindChildOfType<Time>()->DeltaTime();
      //I need to actually see the logs for now
      //Aspen::Log::Info("%f", timer);

      //I'm lazy don't judge
      bool aDown = Aspen::Input::KeyHeld(SDLK_a);
      bool dDown = Aspen::Input::KeyHeld(SDLK_d);
      //Create floor
      if(GetTransform()->GetYPosition()>400){
        jumping = false;
        yVelocity = 0;
        GetTransform()->SetYPosition(400);
      }




      //Roll Script
      if(rolling){
        if(rollFrame>=dashFrames){
          CurrentAnimation->GetTransform()->SetRotation(0);
          rolling = false;
          rollFrame = 0;
        }
        else{
          if(rightFacing){
            xVelocity = rollSpeed;
            CurrentAnimation->GetTransform()->SetRotation(CurrentAnimation->GetTransform()->GetRotation() + rollRotationPerFrame);
          }
          else{
            xVelocity = -rollSpeed;
            CurrentAnimation->GetTransform()->SetRotation(CurrentAnimation->GetTransform()->GetRotation() - rollRotationPerFrame);
          }
          Aspen::Log::Info("%f", xVelocity);
          rollFrame++;
        }
      }
      //Blocking Script + Blocked Start
      else if(blocking){
        blockFrame++;
        if(blockFrame==3*5){
          blockFrame = 0;
          blocking = false;
          blocked = true;
        }
      }
      //Block Script
      else if(blocked){
        if(Aspen::Input::KeyHeld(SDLK_s)){
          blocked = true;
        }
        else{
          blocked = false;
        }
        //Determine facing
        if(aDown){
          rightFacing = false;
        }
        if(dDown){
          rightFacing = true;
        }
      }
      //Guardbreak Script
      else if(guardBreaking){
        guardBreakFrame++;
        if(guardBreakFrame == 8*5){
          guardBreakFrame = 0;
          guardBreaking = false;
        }
      }
      //Shoot Script
      else if(shooting){
        shootFrame++;
        if(shootFrame == 10*5){
          shooting = false;
          shootFrame = 0;
        }
      }
      //Attacc script
      else if(attacking){
        attackFrame++;
        if(attackFrame==4*5){
          attacking = false;
          attackFrame = 0;
        }
      }
      //Movement Scripts
      else{
        //Run Scripts
        if(aDown){
          Aspen::Log::Info("A is held");
          xVelocity = -runxVelocity;
          rightFacing = false;
        }
        if(dDown){
          Aspen::Log::Info("D is held");
          xVelocity = runxVelocity;
          rightFacing = true;
        }
        if(!aDown && !dDown){
          xVelocity = 0;
          running = false;
        }
        else{
          running = true;
        }
        //Dash Scripts
        if(Aspen::Input::KeyPressed(SDLK_q) && dash == false){
          dash = true;
          startDash = timer;
        }
        if(dash){
          xVelocity *= dashMultiplier;
        }
        if(timer-startDash>=dashLength){
          dash = false;
        }
        //Jump Scripts
        if(Aspen::Input::KeyHeld(SDLK_w) && jumping == false){
          yVelocity = jumpStrength;
          jumping = true;
        }
        if (jumping){
          yVelocity+=gravity;
          Aspen::Log::Info("%f", yVelocity);
        }

      }

      //Starting things
      //Starting rolls (They get special treatment since you can roll out of block)
      //Put after the movement scripts since otherwise you could cancel anything frame 1
      if(!attacking && !guardBreaking && !shooting){
        if(Aspen::Input::KeyPressed(SDLK_e)){
          rolling = true;
          blocking = false;
        }
        //Starting blocks
        else if(Aspen::Input::KeyHeld(SDLK_s) && !blocked && !blocking){
          blocking = true;
        }
        //Starting Guardbreaks
        else if(Aspen::Input::KeyPressed(SDLK_r) && guardBreaking == false && !blocked){
          guardBreaking = true;
        }
        //Starting G U N
        else if(Aspen::Input::KeyPressed(SDLK_1) && !blocked){
          shooting = true;
        }
        //Starting attacc
        else if(Aspen::Input::KeyPressed(SDLK_f)){
          attacking = true;
        }
      }
      



      //Animation gargbag
      if(rolling){
        CurrentAnimation->Deactivate();
        CurrentAnimation=Rolling;
        CurrentAnimation->Activate();
      }
      else if(blocking){
        CurrentAnimation->Deactivate();
        CurrentAnimation=Blocking;
        CurrentAnimation->Activate();
      }
      else if(blocked){
        CurrentAnimation->Deactivate();
        CurrentAnimation=Blocked;
        CurrentAnimation->Activate();
      }
      else if(guardBreaking){
        CurrentAnimation->Deactivate();
        CurrentAnimation=Guardbreak;
        CurrentAnimation->Activate();
      }
      else if(shooting){
        CurrentAnimation->Deactivate();
        CurrentAnimation=Shooting;
        CurrentAnimation->Activate();
      }
      else if(attacking){
        CurrentAnimation->Deactivate();
        CurrentAnimation=Attacking;
        CurrentAnimation->Activate();
      }
      else if(running || jumping){
        CurrentAnimation->Deactivate();
        CurrentAnimation=Running;
        CurrentAnimation->Activate();
      }
      else{
        CurrentAnimation->Deactivate();
        CurrentAnimation=Idle;
        CurrentAnimation->Activate();
      }
      if(rightFacing){
        CurrentAnimation->GetTransform()->SetScale(1,1);
      }
      else{
        CurrentAnimation->GetTransform()->SetScale(-1,1);
      }
      CurrentAnimation->Deactivate();
      CurrentAnimation->Activate();
      GetRigidbody()->SetCartesianVelocity(xVelocity,yVelocity);

    }
};
class MainMenu : public GameState
{
  Aspen::Graphics::UI::Text *title;
  Player1 *player1;
  Wall *floor;
  Wall *leftWall;
public:
  MainMenu(Object *parent = nullptr, std::string name = "MainMenu") : GameState(parent, name)
  {
    //Create title
    title = new Aspen::Graphics::UI::Text("Cats lol", "default", 64, this, "Title");
    AddChild(title);
    title->GetTransform()->SetPosition(100, 223);

    //Create player 1
    player1 = new Player1();
    AddChild(player1);

    //Floor
    floor = new Wall();
    AddChild(floor);
    floor->GetTransform()->SetPosition(83, 235);
    floor->GetTransform()->SetScale(6.2, 2.66);

    leftWall = new Wall();
    AddChild(leftWall);
  }

  void OnUpdate()
  {
    

  }
};

class Game : public GameState
{
  
};

  

int main(int argc, char **argv)
{
  Aspen::Log::Log::SetFile("./Aspen.log");

  Engine engine(Aspen::Engine::START_FLAGS::ALL ^ (
    Aspen::Engine::START_FLAGS::CREATE_GRAPHICS |
    Aspen::Engine::START_FLAGS::CREATE_GRAPHICS_DEBUGGER |
    Aspen::Engine::START_FLAGS::CREATE_GRAPHICS_FONTCACHE
  ));
  Aspen::Graphics::Graphics *gfx = new Aspen::Graphics::Graphics(1080, 720);
  gfx->CreateChild<Aspen::Debug::Debug>();
  gfx->CreateChild<Aspen::Graphics::FontCache>();
  engine.AddChild(gfx);

  //engine.FindChildOfType<Aspen::Physics::Physics>()->SetGravityStrength(0);
  engine.FindChildOfType<Aspen::Physics::Physics>()->SetDrag(0.1);
  engine.FindChildOfType<Aspen::Time::Time>()->TargetFramerate(60);
  engine.FindChildOfType<Aspen::Graphics::Graphics>()->FindChildOfType<Aspen::Graphics::FontCache>()->LoadFont("resources/ABeeZee-Regular.ttf", "default");

  engine.FindChildOfType<GameStateManager>()->LoadState<MainMenu>(true);
  //engine.FindChildOfType<GameStateManager>()->LoadState<Game>(false);

  while (engine)
    engine();
  return 0;
}
