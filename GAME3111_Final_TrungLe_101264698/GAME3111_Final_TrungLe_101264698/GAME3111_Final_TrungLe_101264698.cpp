﻿//***************************************************************************************
// GAME3111_A2_TrungLe_MehraraSarabi
// Mehrara Sarabi 101247463
// Trung Le 101264698
//
// Hold down '1' key to view scene in wireframe mode.
//***************************************************************************************

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"
#include "../../Common/Camera.h"
#include "FrameResource.h"
#include "Waves.h"
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */


using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

enum class ShapeType {
    kBox = 0,
    kSphere,
    kGeosphere,
    kCylinder,
    kGrid,
    kQuad,
    kDiamond,
    kUndefined
};

enum class RenderLayer : int
{
    Opaque = 0,
    Transparent,
    AlphaTested,
    AlphaTestedTreeSprites,
    Count
};

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
    RenderItem() = default;
    RenderItem(const RenderItem& rhs) = delete;

    // World matrix of the shape that describes the object's local space
    // relative to the world space, which defines the position, orientation,
    // and scale of the object in the world.
    XMFLOAT4X4 World = MathHelper::Identity4x4();

    XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

    // Dirty flag indicating the object data has changed and we need to update the constant buffer.
    // Because we have an object cbuffer for each FrameResource, we have to apply the
    // update to each FrameResource.  Thus, when we modify obect data we should set 
    // NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
    int NumFramesDirty = gNumFrameResources;

    // Index into GPU constant buffer corresponding to the ObjectCB for this render item.
    UINT ObjCBIndex = -1;

    Material* Mat = nullptr;
    MeshGeometry* Geo = nullptr;

    // Primitive topology.
    //D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    BoundingBox Bounds;
    //std::vector<InstanceData> Instances; //not used

    // DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    //step 0
    //UINT InstanceCount = 0; //not used
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

class ShapesApp : public D3DApp
{
public:
    ShapesApp(HINSTANCE hInstance);
    ShapesApp(const ShapesApp& rhs) = delete;
    ShapesApp& operator=(const ShapesApp& rhs) = delete;
    ~ShapesApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
    void UpdateCamera(const GameTimer& gt);
    void AnimateMaterials(const GameTimer& gt);
    void UpdateObjectCBs(const GameTimer& gt);
    void UpdateMaterialCBs(const GameTimer& gt);
    void UpdateInstanceData(const GameTimer& gt); //not used
    void UpdateMaterialBuffer(const GameTimer& gt); //not used
    void UpdateMainPassCB(const GameTimer& gt);
    void UpdateWaves(const GameTimer& gt);

    void LoadTextures();
    void BuildRootSignature();
    void BuildDescriptorHeaps();
    void BuildShadersAndInputLayout();
    void BuildLandGeometry();
    void BuildWavesGeometry();
    void BuildSkullGeometry(); //not used
    void BuildOneShapeGeometry(std::string shape_type, std::string shape_name, float param_a, float param_b, float param_c, float param_d = -999, float param_e = -999);
    void BuildShapeGeometry();
    void BuildTreeSpritesGeometry();
    void BuildCloudSpritesGeometry();
    void BuildWyvernSpritesGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems();
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
    void BuildConstantBufferViews();
    void BuildOneRenderItem(std::string shape_type, std::string shape_name, std::string material, XMMATRIX scale_matrix, XMMATRIX translate_matrix, XMMATRIX tex_scale_matrix, UINT obj_idx);
    void BuildOneRenderItem(std::string shape_type, std::string shape_name, std::string material, XMMATRIX rotate_matrix, XMMATRIX scale_matrix, XMMATRIX translate_matrix, XMMATRIX tex_scale_matrix, UINT obj_idx);

    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

    float GetHillsHeight(float x, float z)const;
    XMFLOAT3 GetHillsNormal(float x, float z)const;
private:

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    UINT mCbvSrvDescriptorSize = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr; //OBSOLETE

    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
    std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

    //std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mStdInputLayout;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout; //not used

    RenderItem* mWavesRitem = nullptr;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;

    // Render items divided by PSO.
    std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
    std::vector<RenderItem*> mOpaqueRitems; //not used
    UINT mInstanceCount = 0; //not used

    std::unique_ptr<Waves> mWaves;

    PassConstants mMainPassCB;

    UINT mPassCbvOffset = 0; //OBSOLETE

    bool mIsWireframe = false;

    XMVECTOR position = XMVectorSet(-20.0f, 70.0f, -120.5f, 0.0f)
        , frontVec = XMVectorSet(0.0f, 0.0f, .0f, 0.0f)
        , worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), upVec, rightVec; // Set by function
    float pitch = -2.8, yaw =4.5f;

    // XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.5f * XM_PI;
    float mPhi = 0.2f * XM_PI;
    float mRadius = 120.0f;

    Camera mCamera;
    float cam_check_dist_ = 0.0f; //dist between cam & obj, will be modified by intersect

    POINT mLastMousePos;

    // MAZE
    const float kMazeWallThickness = 0.2f;
    //const int kMazeHorizontalSize = 304;
    const int kMazeHorizontalSize = 297;
    //const int kMazeVerticalSize = 318;
    const int kMazeVerticalSize = 316;   
    const int kMazeSize = 622;
    std::vector<std::vector<float>> kMazeHorizontalCoords = {
        //////////ROW #0
            {0.0f, 0.0f, 0.0f}, //00
            {1.0f, 0.0f, 0.0f}, //01
            {2.0f, 0.0f, 0.0f}, //02
            {3.0f, 0.0f, 0.0f}, //03
            {4.0f, 0.0f, 0.0f}, //04
            {5.0f, 0.0f, 0.0f}, //05
            {6.0f, 0.0f, 0.0f}, //06
            {7.0f, 0.0f, 0.0f}, //07
            {8.0f, 0.0f, 0.0f}, //08
            {9.0f, 0.0f, 0.0f}, //09
            {10.0f, 0.0f, 0.0f}, //10
            {11.0f, 0.0f, 0.0f}, //11
            //12
            {13.0f, 0.0f, 0.0f}, //13
            {14.0f, 0.0f, 0.0f}, //14
            {15.0f, 0.0f, 0.0f}, //15
            {16.0f, 0.0f, 0.0f}, //16
            {17.0f, 0.0f, 0.0f}, //17
            {18.0f, 0.0f, 0.0f}, //18
            {19.0f, 0.0f, 0.0f}, //19
            {20.0f, 0.0f, 0.0f}, //20
            {21.0f, 0.0f, 0.0f}, //21
            {22.0f, 0.0f, 0.0f}, //22
            {23.0f, 0.0f, 0.0f}, //23
        //////////ROW #1
            {1.0f, 0.0f, -1.0f}, //01
            {4.0f, 0.0f, -1.0f}, //04
            {6.0f, 0.0f, -1.0f}, //06
            {7.0f, 0.0f, -1.0f}, //07
            {10.0f, 0.0f, -1.0f}, //10
            {11.0f, 0.0f, -1.0f}, //11
            {12.0f, 0.0f, -1.0f}, //12
            {17.0f, 0.0f, -1.0f}, //17
            {18.0f, 0.0f, -1.0f}, //18
            {19.0f, 0.0f, -1.0f}, //19
            {22.0f, 0.0f, -1.0f}, //22
        //////////ROW #2
            {5.0f, 0.0f, -2.0f}, //05
            {7.0f, 0.0f, -2.0f}, //07
            {8.0f, 0.0f, -2.0f}, //08
            {11.0f, 0.0f, -2.0f}, //11
            {13.0f, 0.0f, -2.0f}, //13
            {14.0f, 0.0f, -2.0f}, //14
            {21.0f, 0.0f, -2.0f}, //21
        //////////ROW #3
            {3.0f, 0.0f, -3.0f}, //03
            {6.0f, 0.0f, -3.0f}, //06
            {8.0f, 0.0f, -3.0f}, //08
            {9.0f, 0.0f, -3.0f}, //09
            {12.0f, 0.0f, -3.0f}, //12
            {13.0f, 0.0f, -3.0f}, //13
            {14.0f, 0.0f, -3.0f}, //14
            {15.0f, 0.0f, -3.0f}, //15
            {16.0f, 0.0f, -3.0f}, //16
            {17.0f, 0.0f, -3.0f}, //17
            {20.0f, 0.0f, -3.0f}, //20
            {22.0f, 0.0f, -3.0f}, //22
            {23.0f, 0.0f, -3.0f}, //23
        //////////ROW #4
            {0.0f, 0.0f, -4.0f}, //00
            {1.0f, 0.0f, -4.0f}, //01
            {2.0f, 0.0f, -4.0f}, //02
            {5.0f, 0.0f, -4.0f}, //05
            {8.0f, 0.0f, -4.0f}, //08
            {9.0f, 0.0f, -4.0f}, //09
            {10.0f, 0.0f, -4.0f}, //10
            {11.0f, 0.0f, -4.0f}, //11
            {14.0f, 0.0f, -4.0f}, //14
            {15.0f, 0.0f, -4.0f}, //15
            {16.0f, 0.0f, -4.0f}, //16
            {20.0f, 0.0f, -4.0f}, //20
            {21.0f, 0.0f, -4.0f}, //21
            {22.0f, 0.0f, -4.0f}, //22
        //////////ROW #5
            {3.0f, 0.0f, -5.0f}, //03
            {4.0f, 0.0f, -5.0f}, //04
            {5.0f, 0.0f, -5.0f}, //05
            {6.0f, 0.0f, -5.0f}, //06
            {7.0f, 0.0f, -5.0f}, //07
            {11.0f, 0.0f, -5.0f}, //11
            {12.0f, 0.0f, -5.0f}, //12
            {16.0f, 0.0f, -5.0f}, //16
            {17.0f, 0.0f, -5.0f}, //17
            {19.0f, 0.0f, -5.0f}, //19
        //////////ROW #6
            {1.0f, 0.0f, -6.0f}, //01
            {2.0f, 0.0f, -6.0f}, //02
            {4.0f, 0.0f, -6.0f}, //04
            {7.0f, 0.0f, -6.0f}, //07
            {8.0f, 0.0f, -6.0f}, //08
            {20.0f, 0.0f, -6.0f}, //20
            {21.0f, 0.0f, -6.0f}, //21
        //////////ROW #7
            {2.0f, 0.0f, -7.0f}, //02
            {3.0f, 0.0f, -7.0f}, //03
            {6.0f, 0.0f, -7.0f}, //06
            {8.0f, 0.0f, -7.0f}, //08
            {9.0f, 0.0f, -7.0f}, //09
            {10.0f, 0.0f, -7.0f}, //10
            {11.0f, 0.0f, -7.0f}, //11
            {13.0f, 0.0f, -7.0f}, //13
            {16.0f, 0.0f, -7.0f}, //16
            {17.0f, 0.0f, -7.0f}, //17
            {19.0f, 0.0f, -7.0f}, //19
            {20.0f, 0.0f, -7.0f}, //20
        //////////ROW #8
            {5.0f, 0.0f, -8.0f}, //05
            {7.0f, 0.0f, -8.0f}, //07
            {9.0f, 0.0f, -8.0f}, //09
            {12.0f, 0.0f, -8.0f}, //12
            {14.0f, 0.0f, -8.0f}, //14
            {15.0f, 0.0f, -8.0f}, //15
            {18.0f, 0.0f, -8.0f}, //18
            {21.0f, 0.0f, -8.0f}, //21
            {22.0f, 0.0f, -8.0f}, //22
        //////////ROW #9
            {2.0f, 0.0f, -9.0f}, //02
            {3.0f, 0.0f, -9.0f}, //03
            {6.0f, 0.0f, -9.0f}, //06
            {7.0f, 0.0f, -9.0f}, //07
            {8.0f, 0.0f, -9.0f}, //08
            {10.0f, 0.0f, -9.0f}, //10
            {11.0f, 0.0f, -9.0f}, //11
            {12.0f, 0.0f, -9.0f}, //12
            {13.0f, 0.0f, -9.0f}, //13
            {16.0f, 0.0f, -9.0f}, //16
            {17.0f, 0.0f, -9.0f}, //17
            {19.0f, 0.0f, -9.0f}, //19
            {20.0f, 0.0f, -9.0f}, //20
            {23.0f, 0.0f, -9.0f}, //23
        //////////ROW #10
            {1.0f, 0.0f, -10.0f}, //01
            {3.0f, 0.0f, -10.0f}, //03
            {4.0f, 0.0f, -10.0f}, //04
            {5.0f, 0.0f, -10.0f}, //05
            {9.0f, 0.0f, -10.0f}, //09
            {10.0f, 0.0f, -10.0f}, //10
            {11.0f, 0.0f, -10.0f}, //11
            {14.0f, 0.0f, -10.0f}, //14
            {17.0f, 0.0f, -10.0f}, //17
            {20.0f, 0.0f, -10.0f}, //20
            {21.0f, 0.0f, -10.0f}, //21
            {22.0f, 0.0f, -10.0f}, //22
        //////////ROW #11
            {1.0f, 0.0f, -11.0f}, //01
            {8.0f, 0.0f, -11.0f}, //08
            {12.0f, 0.0f, -11.0f}, //12
            {13.0f, 0.0f, -11.0f}, //13
            {14.0f, 0.0f, -11.0f}, //14
            {15.0f, 0.0f, -11.0f}, //15
            {17.0f, 0.0f, -11.0f}, //17
            {18.0f, 0.0f, -11.0f}, //18
            {23.0f, 0.0f, -11.0f}, //23
        //////////ROW #12
            {2.0f, 0.0f, -12.0f}, //02
            {3.0f, 0.0f, -12.0f}, //03
            {4.0f, 0.0f, -12.0f}, //04
            {6.0f, 0.0f, -12.0f}, //06
            {7.0f, 0.0f, -12.0f}, //07
            {9.0f, 0.0f, -12.0f}, //09
            //{10.0f, 0.0f, -12.0f}, //10
            //{11.0f, 0.0f, -12.0f}, //11
            //{12.0f, 0.0f, -12.0f}, //12
            {19.0f, 0.0f, -12.0f}, //19
            {20.0f, 0.0f, -12.0f}, //20
        //////////ROW #13
            {1.0f, 0.0f, -13.0f}, //01
            {4.0f, 0.0f, -13.0f}, //04
            {5.0f, 0.0f, -13.0f}, //05
            //{10.0f, 0.0f, -13.0f}, //10
            //{11.0f, 0.0f, -13.0f}, //11
            //{12.0f, 0.0f, -13.0f}, //12
            //{13.0f, 0.0f, -13.0f}, //13
            {15.0f, 0.0f, -13.0f}, //15
            {16.0f, 0.0f, -13.0f}, //16
            {17.0f, 0.0f, -13.0f}, //17
            {18.0f, 0.0f, -13.0f}, //18
            {19.0f, 0.0f, -13.0f}, //19
            {22.0f, 0.0f, -13.0f}, //22
        //////////ROW #14
            {0.0f, 0.0f, -14.0f}, //00
            {2.0f, 0.0f, -14.0f}, //02
            {3.0f, 0.0f, -14.0f}, //03
            {6.0f, 0.0f, -14.0f}, //06
            {10.0f, 0.0f, -14.0f}, //10
            {11.0f, 0.0f, -14.0f}, //11
            {12.0f, 0.0f, -14.0f}, //12
            {13.0f, 0.0f, -14.0f}, //13
            {14.0f, 0.0f, -14.0f}, //14
            {15.0f, 0.0f, -14.0f}, //15
            {17.0f, 0.0f, -14.0f}, //17
            {22.0f, 0.0f, -14.0f}, //22
        //////////ROW #15
            {0.0f, 0.0f, -15.0f}, //00
            {1.0f, 0.0f, -15.0f}, //01
            {4.0f, 0.0f, -15.0f}, //04
            {7.0f, 0.0f, -15.0f}, //07
            {8.0f, 0.0f, -15.0f}, //08
            {9.0f, 0.0f, -15.0f}, //09
            {16.0f, 0.0f, -15.0f}, //16
            {19.0f, 0.0f, -15.0f}, //19
            {20.0f, 0.0f, -15.0f}, //20
            {23.0f, 0.0f, -15.0f}, //23
        //////////ROW #16
            {2.0f, 0.0f, -16.0f}, //02
            {3.0f, 0.0f, -16.0f}, //03
            {4.0f, 0.0f, -16.0f}, //04
            {5.0f, 0.0f, -16.0f}, //05
            {8.0f, 0.0f, -16.0f}, //08
            {9.0f, 0.0f, -16.0f}, //09
            {10.0f, 0.0f, -16.0f}, //10
            {13.0f, 0.0f, -16.0f}, //13
            {15.0f, 0.0f, -16.0f}, //15
            {17.0f, 0.0f, -16.0f}, //17
            {18.0f, 0.0f, -16.0f}, //18
            {20.0f, 0.0f, -16.0f}, //20
        //////////ROW #17
            {3.0f, 0.0f, -17.0f}, //03
            {4.0f, 0.0f, -17.0f}, //04
            {7.0f, 0.0f, -17.0f}, //07
            {10.0f, 0.0f, -17.0f}, //10
            {11.0f, 0.0f, -17.0f}, //11
            {12.0f, 0.0f, -17.0f}, //12
            {13.0f, 0.0f, -17.0f}, //13
            {14.0f, 0.0f, -17.0f}, //14
            {16.0f, 0.0f, -17.0f}, //16
            {19.0f, 0.0f, -17.0f}, //19
            {22.0f, 0.0f, -17.0f}, //22
        //////////ROW #18
            {1.0f, 0.0f, -18.0f}, //01
            {2.0f, 0.0f, -18.0f}, //02
            {4.0f, 0.0f, -18.0f}, //04
            {5.0f, 0.0f, -18.0f}, //05
            {6.0f, 0.0f, -18.0f}, //06
            {8.0f, 0.0f, -18.0f}, //08
            {9.0f, 0.0f, -18.0f}, //09
            {11.0f, 0.0f, -18.0f}, //11
            {12.0f, 0.0f, -18.0f}, //12
            {13.0f, 0.0f, -18.0f}, //13
            {14.0f, 0.0f, -18.0f}, //14
            {15.0f, 0.0f, -18.0f}, //15
            {17.0f, 0.0f, -18.0f}, //17
            {18.0f, 0.0f, -18.0f}, //18
            {19.0f, 0.0f, -18.0f}, //19
            {21.0f, 0.0f, -18.0f}, //21
        //////////ROW #19
            {0.0f, 0.0f, -19.0f}, //00
            {1.0f, 0.0f, -19.0f}, //01
            {3.0f, 0.0f, -19.0f}, //03
            {4.0f, 0.0f, -19.0f}, //04
            {6.0f, 0.0f, -19.0f}, //06
            {7.0f, 0.0f, -19.0f}, //07
            {16.0f, 0.0f, -19.0f}, //16
            {17.0f, 0.0f, -19.0f}, //17
            {20.0f, 0.0f, -19.0f}, //20
            {22.0f, 0.0f, -19.0f}, //22
        //////////ROW #20
            {3.0f, 0.0f, -20.0f}, //03
            {5.0f, 0.0f, -20.0f}, //05
            {6.0f, 0.0f, -20.0f}, //06
            {11.0f, 0.0f, -20.0f}, //11
            {12.0f, 0.0f, -20.0f}, //12
            {21.0f, 0.0f, -20.0f}, //21
            {23.0f, 0.0f, -20.0f}, //23
        //////////ROW #21
            {1.0f, 0.0f, -21.0f}, //01
            {4.0f, 0.0f, -21.0f}, //04
            {6.0f, 0.0f, -21.0f}, //06
            {7.0f, 0.0f, -21.0f}, //07
            {9.0f, 0.0f, -21.0f}, //09
            {10.0f, 0.0f, -21.0f}, //10
            {14.0f, 0.0f, -21.0f}, //14
            {15.0f, 0.0f, -21.0f}, //15
            {16.0f, 0.0f, -21.0f}, //16
            {17.0f, 0.0f, -21.0f}, //17
            {18.0f, 0.0f, -21.0f}, //18
            {22.0f, 0.0f, -21.0f}, //22
        //////////ROW #22
            {2.0f, 0.0f, -22.0f}, //02
            {4.0f, 0.0f, -22.0f}, //04
            {8.0f, 0.0f, -22.0f}, //08
            {11.0f, 0.0f, -22.0f}, //11
            {12.0f, 0.0f, -22.0f}, //12
            {13.0f, 0.0f, -22.0f}, //13
            {14.0f, 0.0f, -22.0f}, //14
            {15.0f, 0.0f, -22.0f}, //15
            {16.0f, 0.0f, -22.0f}, //16
            {17.0f, 0.0f, -22.0f}, //17
            {18.0f, 0.0f, -22.0f}, //18
            {19.0f, 0.0f, -22.0f}, //19
            {21.0f, 0.0f, -22.0f}, //21
            {22.0f, 0.0f, -22.0f}, //22
            {23.0f, 0.0f, -22.0f}, //23
        //////////ROW #23
            {1.0f, 0.0f, -23.0f}, //01
            {3.0f, 0.0f, -23.0f}, //03
            {5.0f, 0.0f, -23.0f}, //05
            {6.0f, 0.0f, -23.0f}, //06
            {7.0f, 0.0f, -23.0f}, //07
            {8.0f, 0.0f, -23.0f}, //08
            {9.0f, 0.0f, -23.0f}, //09
            {10.0f, 0.0f, -23.0f}, //10
            {11.0f, 0.0f, -23.0f}, //11
            {20.0f, 0.0f, -23.0f}, //20
            {22.0f, 0.0f, -23.0f}, //22
        //////////ROW #24
            {0.0f, 0.0f, -24.0f}, //00
            {1.0f, 0.0f, -24.0f}, //01
            {2.0f, 0.0f, -24.0f}, //02
            {3.0f, 0.0f, -24.0f}, //03
            {4.0f, 0.0f, -24.0f}, //04
            {5.0f, 0.0f, -24.0f}, //05
            {6.0f, 0.0f, -24.0f}, //06
            {7.0f, 0.0f, -24.0f}, //07
            {8.0f, 0.0f, -24.0f}, //08
            {9.0f, 0.0f, -24.0f}, //09
            {10.0f, 0.0f, -24.0f}, //10
            //11
            {12.0f, 0.0f, -24.0f}, //12
            {13.0f, 0.0f, -24.0f}, //13
            {14.0f, 0.0f, -24.0f}, //14
            {15.0f, 0.0f, -24.0f}, //15
            {16.0f, 0.0f, -24.0f}, //16
            {17.0f, 0.0f, -24.0f}, //17
            {18.0f, 0.0f, -24.0f}, //18
            {19.0f, 0.0f, -24.0f}, //19
            {20.0f, 0.0f, -24.0f}, //20
            {21.0f, 0.0f, -24.0f}, //21
            {22.0f, 0.0f, -24.0f}, //22
        { 23.0f, 0.0f, -24.0f } //23
    };
    std::vector<std::vector<float>> kMazeVerticalCoords = {
        //////////COL #0
            {0.0f, 0.0f, -0.0f}, //00
            {0.0f, 0.0f, -1.0f}, //01
            {0.0f, 0.0f, -2.0f}, //02
            {0.0f, 0.0f, -3.0f}, //03
            {0.0f, 0.0f, -4.0f}, //04
            {0.0f, 0.0f, -5.0f}, //05
            {0.0f, 0.0f, -6.0f}, //06
            {0.0f, 0.0f, -7.0f}, //07
            {0.0f, 0.0f, -8.0f}, //08
            {0.0f, 0.0f, -9.0f}, //09
            {0.0f, 0.0f, -10.0f}, //10
            {0.0f, 0.0f, -11.0f}, //11
            {0.0f, 0.0f, -12.0f}, //12
            {0.0f, 0.0f, -13.0f}, //13
            {0.0f, 0.0f, -14.0f}, //14
            {0.0f, 0.0f, -15.0f}, //15
            {0.0f, 0.0f, -16.0f}, //16
            {0.0f, 0.0f, -17.0f}, //17
            {0.0f, 0.0f, -18.0f}, //18
            {0.0f, 0.0f, -19.0f}, //19
            {0.0f, 0.0f, -20.0f}, //20
            {0.0f, 0.0f, -21.0f}, //21
            {0.0f, 0.0f, -22.0f}, //22
            {0.0f, 0.0f, -23.0f}, //23
        //////////COL #1
            {1.0f, 0.0f, -1.0f}, //01
            {1.0f, 0.0f, -2.0f}, //02
            {1.0f, 0.0f, -5.0f}, //05
            {1.0f, 0.0f, -6.0f}, //06
            {1.0f, 0.0f, -7.0f}, //07
            {1.0f, 0.0f, -8.0f}, //08
            {1.0f, 0.0f, -11.0f}, //11
            {1.0f, 0.0f, -12.0f}, //12
            {1.0f, 0.0f, -16.0f}, //16
            {1.0f, 0.0f, -17.0f}, //17
            {1.0f, 0.0f, -20.0f}, //20
            {1.0f, 0.0f, -21.0f}, //21
            {1.0f, 0.0f, -22.0f}, //22
        //////////COL #2
            {2.0f, 0.0f, -1.0f}, //01
            {2.0f, 0.0f, -2.0f}, //02
            {2.0f, 0.0f, -3.0f}, //03
            {2.0f, 0.0f, -4.0f}, //04
            {2.0f, 0.0f, -7.0f}, //07
            {2.0f, 0.0f, -8.0f}, //08
            {2.0f, 0.0f, -9.0f}, //09
            {2.0f, 0.0f, -10.0f}, //10
            {2.0f, 0.0f, -13.0f}, //13
            {2.0f, 0.0f, -14.0f}, //14
            {2.0f, 0.0f, -16.0f}, //16
            {2.0f, 0.0f, -19.0f}, //19
            {2.0f, 0.0f, -23.0f}, //23
        //////////COL #3
            {3.0f, 0.0f, -0.0f}, //00
            {3.0f, 0.0f, -1.0f}, //01
            {3.0f, 0.0f, -5.0f}, //05
            {3.0f, 0.0f, -7.0f}, //07
            {3.0f, 0.0f, -10.0f}, //10
            {3.0f, 0.0f, -11.0f}, //11
            {3.0f, 0.0f, -12.0f}, //12
            {3.0f, 0.0f, -15.0f}, //15
            {3.0f, 0.0f, -17.0f}, //17
            {3.0f, 0.0f, -18.0f}, //18
            {3.0f, 0.0f, -19.0f}, //19
            {3.0f, 0.0f, -20.0f}, //20
            {3.0f, 0.0f, -21.0f}, //21
            {3.0f, 0.0f, -22.0f}, //22
        //////////COL #4
            {4.0f, 0.0f, -1.0f}, //01
            {4.0f, 0.0f, -2.0f}, //02
            {4.0f, 0.0f, -3.0f}, //03
            {4.0f, 0.0f, -4.0f}, //04
            {4.0f, 0.0f, -6.0f}, //06
            {4.0f, 0.0f, -8.0f}, //08
            {4.0f, 0.0f, -11.0f}, //11
            {4.0f, 0.0f, -13.0f}, //13
            {4.0f, 0.0f, -14.0f}, //14
        //////////COL #5
            {5.0f, 0.0f, -2.0f}, //02
            {5.0f, 0.0f, -3.0f}, //03
            {5.0f, 0.0f, -7.0f}, //07
            {5.0f, 0.0f, -8.0f}, //08
            {5.0f, 0.0f, -9.0f}, //09
            {5.0f, 0.0f, -10.0f}, //10
            {5.0f, 0.0f, -14.0f}, //14
            {5.0f, 0.0f, -18.0f}, //18
            {5.0f, 0.0f, -20.0f}, //20
            {5.0f, 0.0f, -21.0f}, //21
            {5.0f, 0.0f, -22.0f}, //22
        //////////COL #6
            {6.0f, 0.0f, -0.0f}, //00
            {6.0f, 0.0f, -1.0f}, //01
            {6.0f, 0.0f, -6.0f}, //06
            {6.0f, 0.0f, -10.0f}, //10
            {6.0f, 0.0f, -11.0f}, //11
            {6.0f, 0.0f, -12.0f}, //12
            {6.0f, 0.0f, -14.0f}, //14
            {6.0f, 0.0f, -15.0f}, //15
            {6.0f, 0.0f, -16.0f}, //16
            {6.0f, 0.0f, -17.0f}, //17
            {6.0f, 0.0f, -19.0f}, //19
            {6.0f, 0.0f, -21.0f}, //21
        //////////COL #7
            {7.0f, 0.0f, -2.0f}, //02
            {7.0f, 0.0f, -4.0f}, //04
            {7.0f, 0.0f, -7.0f}, //07
            {7.0f, 0.0f, -9.0f}, //09
            {7.0f, 0.0f, -10.0f}, //10
            {7.0f, 0.0f, -13.0f}, //13
            {7.0f, 0.0f, -15.0f}, //15
            {7.0f, 0.0f, -16.0f}, //16
            {7.0f, 0.0f, -17.0f}, //17
            {7.0f, 0.0f, -22.0f}, //22
        //////////COL #8
            {8.0f, 0.0f, -3.0f}, //03
            {8.0f, 0.0f, -4.0f}, //04
            {8.0f, 0.0f, -6.0f}, //06
            {8.0f, 0.0f, -7.0f}, //07
            {8.0f, 0.0f, -10.0f}, //10
            {8.0f, 0.0f, -11.0f}, //11
            {8.0f, 0.0f, -12.0f}, //12
            {8.0f, 0.0f, -13.0f}, //13
            {8.0f, 0.0f, -18.0f}, //18
            {8.0f, 0.0f, -19.0f}, //19
            {8.0f, 0.0f, -20.0f}, //20
            {8.0f, 0.0f, -22.0f}, //22
        //////////COL #9
            {9.0f, 0.0f, -0.0f}, //00
            {9.0f, 0.0f, -1.0f}, //01
            {9.0f, 0.0f, -5.0f}, //05
            {9.0f, 0.0f, -8.0f}, //08
            {9.0f, 0.0f, -9.0f}, //09
            {9.0f, 0.0f, -12.0f}, //12
            {9.0f, 0.0f, -14.0f}, //14
            {9.0f, 0.0f, -17.0f}, //17
            {9.0f, 0.0f, -19.0f}, //19
            {9.0f, 0.0f, -20.0f}, //20
        //////////COL #10
            {10.0f, 0.0f, -1.0f}, //01
            {10.0f, 0.0f, -2.0f}, //02
            {10.0f, 0.0f, -4.0f}, //04
            {10.0f, 0.0f, -5.0f}, //05
            {10.0f, 0.0f, -6.0f}, //06
            {10.0f, 0.0f, -10.0f}, //10
            {10.0f, 0.0f, -11.0f}, //11
            {10.0f, 0.0f, -13.0f}, //13
            {10.0f, 0.0f, -14.0f}, //14
            {10.0f, 0.0f, -17.0f}, //17
            {10.0f, 0.0f, -18.0f}, //18
            {10.0f, 0.0f, -19.0f}, //19
            {10.0f, 0.0f, -20.0f}, //20
            {10.0f, 0.0f, -21.0f}, //21
            {10.0f, 0.0f, -22.0f}, //22
        //////////COL #11
            {11.0f, 0.0f, -3.0f}, //03
            {11.0f, 0.0f, -5.0f}, //05
            {11.0f, 0.0f, -7.0f}, //07
            {11.0f, 0.0f, -8.0f}, //08
            {11.0f, 0.0f, -10.0f}, //10
            {11.0f, 0.0f, -15.0f}, //15
            {11.0f, 0.0f, -18.0f}, //18
            {11.0f, 0.0f, -19.0f}, //19
            {11.0f, 0.0f, -23.0f}, //23
        //////////COL #12
            {12.0f, 0.0f, -1.0f}, //01
            {12.0f, 0.0f, -2.0f}, //02
            {12.0f, 0.0f, -6.0f}, //06
            {12.0f, 0.0f, -7.0f}, //07
            //{12.0f, 0.0f, -11.0f}, //11
            {12.0f, 0.0f, -15.0f}, //15
            {12.0f, 0.0f, -16.0f}, //16
            {12.0f, 0.0f, -18.0f}, //18
            {12.0f, 0.0f, -20.0f}, //20
            {12.0f, 0.0f, -21.0f}, //21
        //////////COL #13
            {13.0f, 0.0f, -3.0f}, //03
            {13.0f, 0.0f, -4.0f}, //04
            {13.0f, 0.0f, -5.0f}, //05
            {13.0f, 0.0f, -9.0f}, //09
            {13.0f, 0.0f, -10.0f}, //10
            {13.0f, 0.0f, -14.0f}, //14
            {13.0f, 0.0f, -19.0f}, //19
            {13.0f, 0.0f, -20.0f}, //20
            {13.0f, 0.0f, -22.0f}, //22
            {13.0f, 0.0f, -23.0f}, //23
        //////////COL #14
            {14.0f, 0.0f, -1.0f}, //01
            {14.0f, 0.0f, -4.0f}, //04
            {14.0f, 0.0f, -5.0f}, //05
            {14.0f, 0.0f, -6.0f}, //06
            {14.0f, 0.0f, -7.0f}, //07
            {14.0f, 0.0f, -9.0f}, //09
            //{14.0f, 0.0f, -12.0f}, //12
            {14.0f, 0.0f, -15.0f}, //15
            {14.0f, 0.0f, -16.0f}, //16
            {14.0f, 0.0f, -18.0f}, //18
            {14.0f, 0.0f, -20.0f}, //20
            {14.0f, 0.0f, -23.0f}, //23
        //////////COL #15
            {15.0f, 0.0f, -0.0f}, //00
            {15.0f, 0.0f, -1.0f}, //01
            {15.0f, 0.0f, -4.0f}, //04
            {15.0f, 0.0f, -5.0f}, //05
            {15.0f, 0.0f, -6.0f}, //06
            {15.0f, 0.0f, -8.0f}, //08
            {15.0f, 0.0f, -11.0f}, //11
            {15.0f, 0.0f, -12.0f}, //12
            {15.0f, 0.0f, -14.0f}, //14
            {15.0f, 0.0f, -15.0f}, //15
            {15.0f, 0.0f, -19.0f}, //19
            {15.0f, 0.0f, -20.0f}, //20
            {15.0f, 0.0f, -22.0f}, //22
        //////////COL #16
            {16.0f, 0.0f, -1.0f}, //01
            {16.0f, 0.0f, -2.0f}, //02
            {16.0f, 0.0f, -5.0f}, //05
            {16.0f, 0.0f, -7.0f}, //07
            {16.0f, 0.0f, -9.0f}, //09
            {16.0f, 0.0f, -10.0f}, //10
            {16.0f, 0.0f, -12.0f}, //12
            {16.0f, 0.0f, -14.0f}, //14
            {16.0f, 0.0f, -16.0f}, //16
            {16.0f, 0.0f, -17.0f}, //17
            {16.0f, 0.0f, -18.0f}, //18
            {16.0f, 0.0f, -19.0f}, //19
            {16.0f, 0.0f, -22.0f}, //22
        //////////COL #17
            {17.0f, 0.0f, -0.0f}, //00
            {17.0f, 0.0f, -1.0f}, //01
            {17.0f, 0.0f, -6.0f}, //06
            {17.0f, 0.0f, -8.0f}, //08
            {17.0f, 0.0f, -10.0f}, //10
            {17.0f, 0.0f, -11.0f}, //11
            {17.0f, 0.0f, -13.0f}, //13
            {17.0f, 0.0f, -15.0f}, //15
            {17.0f, 0.0f, -20.0f}, //20
            {17.0f, 0.0f, -23.0f}, //23
        //////////COL #18
            {18.0f, 0.0f, -2.0f}, //02
            {18.0f, 0.0f, -3.0f}, //03
            {18.0f, 0.0f, -4.0f}, //04
            {18.0f, 0.0f, -5.0f}, //05
            {18.0f, 0.0f, -7.0f}, //07
            {18.0f, 0.0f, -8.0f}, //08
            {18.0f, 0.0f, -12.0f}, //12
            {18.0f, 0.0f, -14.0f}, //14
            {18.0f, 0.0f, -17.0f}, //17
            {18.0f, 0.0f, -19.0f}, //19
            {18.0f, 0.0f, -22.0f}, //22
        //////////COL #19
            {19.0f, 0.0f, -1.0f}, //01
            {19.0f, 0.0f, -2.0f}, //02
            {19.0f, 0.0f, -3.0f}, //03
            {19.0f, 0.0f, -4.0f}, //04
            {19.0f, 0.0f, -6.0f}, //06
            {19.0f, 0.0f, -7.0f}, //07
            {19.0f, 0.0f, -9.0f}, //09
            {19.0f, 0.0f, -10.0f}, //10
            {19.0f, 0.0f, -11.0f}, //11
            {19.0f, 0.0f, -14.0f}, //14
            {19.0f, 0.0f, -15.0f}, //15
            {19.0f, 0.0f, -16.0f}, //16
            {19.0f, 0.0f, -18.0f}, //18
            {19.0f, 0.0f, -19.0f}, //19
            {19.0f, 0.0f, -20.0f}, //20
            {19.0f, 0.0f, -23.0f}, //23
        //////////COL #20
            {20.0f, 0.0f, -1.0f}, //01
            {20.0f, 0.0f, -5.0f}, //05
            {20.0f, 0.0f, -8.0f}, //08
            {20.0f, 0.0f, -10.0f}, //10
            {20.0f, 0.0f, -13.0f}, //13
            {20.0f, 0.0f, -17.0f}, //17
            {20.0f, 0.0f, -19.0f}, //19
            {20.0f, 0.0f, -20.0f}, //20
            {20.0f, 0.0f, -21.0f}, //21
        //////////COL #21
            {21.0f, 0.0f, -1.0f}, //01
            {21.0f, 0.0f, -3.0f}, //03
            {21.0f, 0.0f, -4.0f}, //04
            {21.0f, 0.0f, -7.0f}, //07
            {21.0f, 0.0f, -11.0f}, //11
            {21.0f, 0.0f, -12.0f}, //12
            {21.0f, 0.0f, -13.0f}, //13
            {21.0f, 0.0f, -14.0f}, //14
            {21.0f, 0.0f, -16.0f}, //16
            {21.0f, 0.0f, -17.0f}, //17
            {21.0f, 0.0f, -18.0f}, //18
            {21.0f, 0.0f, -20.0f}, //20
            {21.0f, 0.0f, -21.0f}, //21
            {21.0f, 0.0f, -22.0f}, //22
        //////////COL #22
            {22.0f, 0.0f, -0.0f}, //00
            {22.0f, 0.0f, -2.0f}, //02
            {22.0f, 0.0f, -5.0f}, //05
            {22.0f, 0.0f, -6.0f}, //06
            {22.0f, 0.0f, -8.0f}, //08
            {22.0f, 0.0f, -9.0f}, //09
            {22.0f, 0.0f, -10.0f}, //10
            {22.0f, 0.0f, -11.0f}, //11
            {22.0f, 0.0f, -12.0f}, //12
            {22.0f, 0.0f, -14.0f}, //14
            {22.0f, 0.0f, -15.0f}, //15
            {22.0f, 0.0f, -16.0f}, //16
            {22.0f, 0.0f, -19.0f}, //19
            {22.0f, 0.0f, -20.0f}, //20
            {22.0f, 0.0f, -23.0f}, //23
        //////////COL #23
            {23.0f, 0.0f, -1.0f}, //01
            {23.0f, 0.0f, -4.0f}, //04
            {23.0f, 0.0f, -5.0f}, //05
            {23.0f, 0.0f, -6.0f}, //06
            {23.0f, 0.0f, -7.0f}, //07
            {23.0f, 0.0f, -11.0f}, //11
            {23.0f, 0.0f, -15.0f}, //15
            {23.0f, 0.0f, -17.0f}, //17
            {23.0f, 0.0f, -18.0f}, //18
        //////////COL #24
            {24.0f, 0.0f, -0.0f}, //00
            {24.0f, 0.0f, -1.0f}, //01
            {24.0f, 0.0f, -2.0f}, //02
            {24.0f, 0.0f, -3.0f}, //03
            {24.0f, 0.0f, -4.0f}, //04
            {24.0f, 0.0f, -5.0f}, //05
            {24.0f, 0.0f, -6.0f}, //06
            {24.0f, 0.0f, -7.0f}, //07
            {24.0f, 0.0f, -8.0f}, //08
            {24.0f, 0.0f, -9.0f}, //09
            {24.0f, 0.0f, -10.0f}, //10
            {24.0f, 0.0f, -11.0f}, //11
            {24.0f, 0.0f, -12.0f}, //12
            {24.0f, 0.0f, -13.0f}, //13
            {24.0f, 0.0f, -14.0f}, //14
            {24.0f, 0.0f, -15.0f}, //15
            {24.0f, 0.0f, -16.0f}, //16
            {24.0f, 0.0f, -17.0f}, //17
            {24.0f, 0.0f, -18.0f}, //18
            {24.0f, 0.0f, -19.0f}, //19
            {24.0f, 0.0f, -20.0f}, //20
            {24.0f, 0.0f, -21.0f}, //21
            {24.0f, 0.0f, -22.0f}, //22
            { 24.0f, 0.0f, -23.0f } //23
    };
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        ShapesApp theApp(hInstance);
        if (!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

ShapesApp::ShapesApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

ShapesApp::~ShapesApp()
{
    if (md3dDevice != nullptr)
        FlushCommandQueue();
}

bool ShapesApp::Initialize()
{
    ::OutputDebugStringA(">>> Init started...\n");

    srand(time(NULL));

    if (!D3DApp::Initialize())
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // Get the increment size of a descriptor in this heap type.  This is hardware specific, 
    // so we have to query this information.
    mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    mCamera.SetPosition(0.0f, 2.0f, -50.0f);

    mWaves = std::make_unique<Waves>(240, 240, 1.0f, 0.03f, 4.0f, 0.2f);

    LoadTextures();
    BuildRootSignature();
    BuildDescriptorHeaps();
    BuildShadersAndInputLayout();
    //BuildSkullGeometry();
    BuildLandGeometry();
    BuildWavesGeometry();
    BuildShapeGeometry();
    BuildTreeSpritesGeometry();
    BuildCloudSpritesGeometry();
    BuildWyvernSpritesGeometry();
    BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    //BuildConstantBufferViews();
    BuildPSOs();

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    ::OutputDebugStringA(">>> Init DONE!\n");

    return true;
}

void ShapesApp::OnResize()
{
    D3DApp::OnResize();

    //// The window resized, so update the aspect ratio and recompute the projection matrix.
    //XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    //XMStoreFloat4x4(&mProj, P);

    mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void ShapesApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
    //UpdateCamera(gt);

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    AnimateMaterials(gt);
    UpdateObjectCBs(gt);
    UpdateMaterialCBs(gt);
    ////step1: 	UpdateObjectCBs(gt);
    //UpdateInstanceData(gt);
    //UpdateMaterialBuffer(gt);

    UpdateMainPassCB(gt);
    UpdateWaves(gt);
}

void ShapesApp::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    if (mIsWireframe)
    {
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque_wireframe"].Get()));
    }
    else
    {
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
    }

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    //mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    /*ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    int passCbvIndex = mPassCbvOffset + mCurrFrameResourceIndex;
    auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    passCbvHandle.Offset(passCbvIndex, mCbvSrvUavDescriptorSize);
    mCommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);*/

    ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    //// Bind all the materials used in this scene.  For structured buffers, we can bypass the heap and 
    //// set as a root descriptor.
    //auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
    //mCommandList->SetGraphicsRootShaderResourceView(1, matBuffer->GetGPUVirtualAddress());

    auto passCB = mCurrFrameResource->PassCB->Resource();
    mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    ////step3:  Bind all the textures used in this scene.
    //mCommandList->SetGraphicsRootDescriptorTable(3, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

    mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

    mCommandList->SetPipelineState(mPSOs["treeSprites"].Get());
    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);

    mCommandList->SetPipelineState(mPSOs["transparent"].Get());
    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);

    //DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

    // Indicate a state transition on the resource usage.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    ThrowIfFailed(mCommandList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Advance the fence value to mark commands up to this fence point.
    mCurrFrameResource->Fence = ++mCurrentFence;

    // Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void ShapesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void ShapesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void ShapesApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

        mCamera.Pitch(dy);
        mCamera.RotateY(dx);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void ShapesApp::OnKeyboardInput(const GameTimer& gt)
{
    //step4: using camera class
    const float dt = gt.DeltaTime();

    if (GetAsyncKeyState('W') & 0x8000)
        mCamera.Walk(20.0f * dt);

    if (GetAsyncKeyState('S') & 0x8000)
        mCamera.Walk(-20.0f * dt);

    if (GetAsyncKeyState('A') & 0x8000)
        mCamera.Strafe(-20.0f * dt);

    if (GetAsyncKeyState('D') & 0x8000)
        mCamera.Strafe(20.0f * dt);

    mCamera.UpdateViewMatrix();


    

    for (int i = 0; i < mRitemLayer[(int)RenderLayer::Opaque].size(); i++)
    {
        if (mRitemLayer[(int)RenderLayer::Opaque][i]->Bounds.Intersects(mCamera.GetPosition(), mCamera.GetLook(), cam_check_dist_)) {
            if (cam_check_dist_ < 5.0f) { //HIT
                /*std::wstring text2 =
                    L"hit id = " + std::to_wstring(i) + L" " +
                    L"\n";
                ::OutputDebugString(text2.c_str());

                ::OutputDebugStringA(">>> mCamera.GetPosition3f() is...\n");
                std::wstring text =
                    L"X = " + std::to_wstring(mCamera.GetPosition3f().x) + L" " +
                    L"Y = " + std::to_wstring(mCamera.GetPosition3f().y) + L" " +
                    L"Z = " + std::to_wstring(mCamera.GetPosition3f().z) + L" " +
                    L"\n";
                ::OutputDebugString(text.c_str());

                ::OutputDebugStringA(">>> mCamera.GetLook3f() is...\n");
                std::wstring text1 =
                    L"X = " + std::to_wstring(mCamera.GetLook3f().x) + L" " +
                    L"Y = " + std::to_wstring(mCamera.GetLook3f().y) + L" " +
                    L"Z = " + std::to_wstring(mCamera.GetLook3f().z) + L" " +
                    L"\n";
                ::OutputDebugString(text1.c_str());*/
            }
        }
    }

    //for (auto& i: mRitemLayer[(int)RenderLayer::Opaque])
    //{
    //    if (mRitemLayer[(int)RenderLayer::Opaque][i]->Bounds.Intersects(mCamera.GetPosition(), mCamera.GetLook(), cam_check_dist_)) {
    //        ::OutputDebugStringA(">>> HIT !!!\n");
    //    }
    //}
       
    /*::OutputDebugStringA(">>> mCamera.GetPosition3f() is...\n");
    std::wstring text =
        L"X = " + std::to_wstring(mCamera.GetPosition3f().x) + L" " +
        L"Y = " + std::to_wstring(mCamera.GetPosition3f().y) + L" " +
        L"Z = " + std::to_wstring(mCamera.GetPosition3f().z) + L" " +
        L"\n";
    ::OutputDebugString(text.c_str());

    ::OutputDebugStringA(">>> mCamera.GetLook3f() is...\n");
    std::wstring text1 =
        L"X = " + std::to_wstring(mCamera.GetLook3f().x) + L" " +
        L"Y = " + std::to_wstring(mCamera.GetLook3f().y) + L" " +
        L"Z = " + std::to_wstring(mCamera.GetLook3f().z) + L" " +
        L"\n";
    ::OutputDebugString(text1.c_str());*/

}

void ShapesApp::UpdateCamera(const GameTimer& gt)
{
    // Convert Spherical to Cartesian coordinates.
    //mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
    //mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
    //mEyePos.y = mRadius * cosf(mPhi);


    // Build the view matrix.
    //XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
    //XMVECTOR target = XMVectorZero();
    //XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    //XMMATRIX view = XMMatrixLookAtLH(pos, target, up);

    frontVec = XMVectorSet(cos((yaw)) * cos((pitch)), sin((pitch)), sin((yaw)) * cos((pitch)), 0.0f);

    frontVec = XMVector3Normalize(frontVec);
    rightVec = XMVector3Normalize(XMVector3Cross(frontVec, worldUp));
    upVec = XMVector3Normalize(XMVector3Cross(rightVec, frontVec));

    XMMATRIX view = XMMatrixLookAtLH(position, // Camera position
        position + frontVec, // Look target
        upVec); // Up vector);

    XMStoreFloat4x4(&mView, view);
}

void ShapesApp::AnimateMaterials(const GameTimer& gt)
{
    // Scroll the water material texture coordinates.
    auto waterMat = mMaterials["water"].get();

    float& tu = waterMat->MatTransform(3, 0);
    float& tv = waterMat->MatTransform(3, 1);

    tu += 0.1f * gt.DeltaTime();
    tv += 0.02f * gt.DeltaTime();

    if (tu >= 1.0f)
        tu -= 1.0f;

    if (tv >= 1.0f)
        tv -= 1.0f;

    waterMat->MatTransform(3, 0) = tu;
    waterMat->MatTransform(3, 1) = tv;

    // Material has changed, so need to update cbuffer.
    waterMat->NumFramesDirty = gNumFrameResources;
}

void ShapesApp::UpdateObjectCBs(const GameTimer& gt)
{
    auto currObjectCB = mCurrFrameResource->ObjectCB.get();
    for (auto& e : mAllRitems)
    {
        // Only update the cbuffer data if the constants have changed.  
        // This needs to be tracked per frame resource.
        if (e->NumFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&e->World);
            XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

            ObjectConstants objConstants;
            XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
            XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

            currObjectCB->CopyData(e->ObjCBIndex, objConstants);

            // Next FrameResource need to be updated too.
            e->NumFramesDirty--;
        }
    }
}

void ShapesApp::UpdateMaterialCBs(const GameTimer& gt)
{
    auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
    for (auto& e : mMaterials)
    {
        // Only update the cbuffer data if the constants have changed.  If the cbuffer
        // data changes, it needs to be updated for each FrameResource.
        Material* mat = e.second.get();
        if (mat->NumFramesDirty > 0)
        {
            XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

            MaterialConstants matConstants;
            matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
            matConstants.FresnelR0 = mat->FresnelR0;
            matConstants.Roughness = mat->Roughness;
            XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

            currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

            // Next FrameResource need to be updated too.
            mat->NumFramesDirty--;
        }
    }
}
/*
void ShapesApp::UpdateInstanceData(const GameTimer& gt)
{
    XMMATRIX view = mCamera.GetView();
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

    auto currInstanceBuffer = mCurrFrameResource->InstanceBuffer.get();
    for (auto& e : mAllRitems)
    {
        const auto& instanceData = e->Instances;

        int visibleInstanceCount = 0;

        for (UINT i = 0; i < (UINT)instanceData.size(); ++i)
        {
            XMMATRIX world = XMLoadFloat4x4(&instanceData[i].World);
            XMMATRIX texTransform = XMLoadFloat4x4(&instanceData[i].TexTransform);

            XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);

            // View space to the object's local space.
            XMMATRIX viewToLocal = XMMatrixMultiply(invView, invWorld);

            InstanceData data;
            XMStoreFloat4x4(&data.World, XMMatrixTranspose(world));
            XMStoreFloat4x4(&data.TexTransform, XMMatrixTranspose(texTransform));
            data.MaterialIndex = instanceData[i].MaterialIndex;

            // Write the instance data to structured buffer for the visible objects.
            currInstanceBuffer->CopyData(visibleInstanceCount++, data);
        }

        e->InstanceCount = visibleInstanceCount;

        std::wostringstream outs;
        outs.precision(6);
        outs << L"Instancing Demo" <<
            L"    " << e->InstanceCount <<
            L" objects visible out of " << e->Instances.size();
        mMainWndCaption = outs.str();
    }
}

void ShapesApp::UpdateMaterialBuffer(const GameTimer& gt)
{
    auto currMaterialBuffer = mCurrFrameResource->MaterialBuffer.get();
    for (auto& e : mMaterials)
    {
        // Only update the cbuffer data if the constants have changed.  If the cbuffer
        // data changes, it needs to be updated for each FrameResource.
        Material* mat = e.second.get();
        if (mat->NumFramesDirty > 0)
        {
            XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

            MaterialData matData;
            matData.DiffuseAlbedo = mat->DiffuseAlbedo;
            matData.FresnelR0 = mat->FresnelR0;
            matData.Roughness = mat->Roughness;
            XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
            //step6: we have 7 different textures!
            matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;

            currMaterialBuffer->CopyData(mat->MatCBIndex, matData);

            // Next FrameResource need to be updated too.
            mat->NumFramesDirty--;
        }
    }
}
*/
void ShapesApp::UpdateMainPassCB(const GameTimer& gt)
{
    XMMATRIX view = mCamera.GetView();
    XMMATRIX proj = mCamera.GetProj();

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    mMainPassCB.EyePosW = mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
    mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
    mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
    mMainPassCB.Lights[0].Strength = { 0.7f, 0.7f, 0.8f };
    mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
    mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
    mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
    mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };
    // 0-2 direction, 3-10 pointlight , 11-16 spot light
    mMainPassCB.Lights[3].Position = { -25.0f, 50.0f, -25.0f };
    mMainPassCB.Lights[3].Strength = { 5.0f, 1.0f, 5.0f };
    mMainPassCB.Lights[4].Position = { 25.0f, 50.0f, -25.0f };
    mMainPassCB.Lights[4].Strength = { 5.0f, 1.0f, 5.0f };
    mMainPassCB.Lights[5].Position = { -25.0f, 50.0f, 25.0f };
    mMainPassCB.Lights[5].Strength = { 5.0f, 1.0f, 5.0f };
    mMainPassCB.Lights[6].Position = { 25.0f, 50.0f, 25.0f };
    mMainPassCB.Lights[6].Strength = { 5.0f, 1.0f, 5.0f };
    mMainPassCB.Lights[7].Position = { -15.0f, 68.0f, -15.0f };
    mMainPassCB.Lights[7].Strength = { 8.0f, 2.0f, 8.0f };
    mMainPassCB.Lights[8].Position = { 15.0f, 68.0f, -15.0f };
    mMainPassCB.Lights[8].Strength = { 8.0f, 2.0f, 8.0f };
    mMainPassCB.Lights[9].Position = { -15.0f, 68.0f, 15.0f };
    mMainPassCB.Lights[9].Strength = { 8.0f, 2.0f, 8.0f };
    mMainPassCB.Lights[10].Position = { 15.0f, 68.0f, 15.0f };
    mMainPassCB.Lights[10].Strength = { 8.0f, 2.0f, 8.0f };
    mMainPassCB.Lights[11].Position = { 0.0f, 58.0f, -2.0f };
    mMainPassCB.Lights[11].Strength = { 5.0f, 2.0f, 10.0f };

    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}


void ShapesApp::UpdateWaves(const GameTimer& gt)
{
    // Every quarter second, generate a random wave.
    static float t_base = 0.0f;
    if ((mTimer.TotalTime() - t_base) >= 0.25f)
    {
        t_base += 0.25f;

        int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
        int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

        float r = MathHelper::RandF(0.1f, 1.0f); // EDIT WAVE INTENSITY

        mWaves->Disturb(i, j, r);
    }

    // Update the wave simulation.
    mWaves->Update(gt.DeltaTime());

    // Update the wave vertex buffer with the new solution.
    auto currWavesVB = mCurrFrameResource->WavesVB.get();
    for (int i = 0; i < mWaves->VertexCount(); ++i)
    {
        Vertex v;
        v.Pos = mWaves->Position(i);
        v.Normal = mWaves->Normal(i);

        // Derive tex-coords from position by 
        // mapping [-w/2,w/2] --> [0,1]
        v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
        v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

        currWavesVB->CopyData(i, v);
    }

    // Set the dynamic VB of the wave renderitem to the current frame VB.
    mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void ShapesApp::LoadTextures() //EDIT TEXTURES HERE
{
    ::OutputDebugStringA(">>> LoadTextures started...\n");

    auto bricksTex = std::make_unique<Texture>();
    bricksTex->Name = "bricksTex";
    bricksTex->Filename = L"../../Textures/bricks.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), bricksTex->Filename.c_str(),
        bricksTex->Resource, bricksTex->UploadHeap));

    auto stoneTex = std::make_unique<Texture>();
    stoneTex->Name = "stoneTex";
    stoneTex->Filename = L"../../Textures/stone.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), stoneTex->Filename.c_str(),
        stoneTex->Resource, stoneTex->UploadHeap));

    auto tileTex = std::make_unique<Texture>();
    tileTex->Name = "tileTex";
    tileTex->Filename = L"../../Textures/groundTex1.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), tileTex->Filename.c_str(),
        tileTex->Resource, tileTex->UploadHeap));

    auto crateTex = std::make_unique<Texture>();
    crateTex->Name = "crateTex";
    crateTex->Filename = L"../../Textures/WoodCrate01.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), crateTex->Filename.c_str(),
        crateTex->Resource, crateTex->UploadHeap));

    auto iceTex = std::make_unique<Texture>();
    iceTex->Name = "iceTex";
    iceTex->Filename = L"../../Textures/ice.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), iceTex->Filename.c_str(),
        iceTex->Resource, iceTex->UploadHeap));

    auto defaultTex = std::make_unique<Texture>();
    defaultTex->Name = "defaultTex";
    defaultTex->Filename = L"../../Textures/white1x1.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), defaultTex->Filename.c_str(),
        defaultTex->Resource, defaultTex->UploadHeap));

    // BOOM
    auto coneTex = std::make_unique<Texture>();
    coneTex->Name = "coneTex";
    coneTex->Filename = L"../../Textures/coneTex.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), coneTex->Filename.c_str(),
        coneTex->Resource, coneTex->UploadHeap));

    auto cylinderTex = std::make_unique<Texture>();
    cylinderTex->Name = "cylinderTex";
    cylinderTex->Filename = L"../../Textures/cylinderTex.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), cylinderTex->Filename.c_str(),
        cylinderTex->Resource, cylinderTex->UploadHeap));


    auto innerBoxTex = std::make_unique<Texture>();
    innerBoxTex->Name = "innerBoxTex";
    innerBoxTex->Filename = L"../../Textures/innerBoxTex.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), innerBoxTex->Filename.c_str(),
        innerBoxTex->Resource, innerBoxTex->UploadHeap));

    auto outerBoxTex = std::make_unique<Texture>();
    outerBoxTex->Name = "outerBoxTex";
    outerBoxTex->Filename = L"../../Textures/outerBoxTex.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), outerBoxTex->Filename.c_str(),
        outerBoxTex->Resource, outerBoxTex->UploadHeap));


    auto diamondTex = std::make_unique<Texture>();
    diamondTex->Name = "diamondTex";
    diamondTex->Filename = L"../../Textures/diamondTex.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), diamondTex->Filename.c_str(),
        diamondTex->Resource, diamondTex->UploadHeap));


    auto cutPyramidTex = std::make_unique<Texture>();
    cutPyramidTex->Name = "cutPyramidTex";
    cutPyramidTex->Filename = L"../../Textures/cutPyramidTex.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), cutPyramidTex->Filename.c_str(),
        cutPyramidTex->Resource, cutPyramidTex->UploadHeap));

    auto holoTex = std::make_unique<Texture>();
    holoTex->Name = "holoTex";
    holoTex->Filename = L"../../Textures/holoTex.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), holoTex->Filename.c_str(),
        holoTex->Resource, holoTex->UploadHeap));

    auto redTex = std::make_unique<Texture>();
    redTex->Name = "redTex";
    redTex->Filename = L"../../Textures/redTex.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), redTex->Filename.c_str(),
        redTex->Resource, redTex->UploadHeap));

    auto cyanTex = std::make_unique<Texture>();
    cyanTex->Name = "cyanTex";
    cyanTex->Filename = L"../../Textures/cyanTex.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), cyanTex->Filename.c_str(),
        cyanTex->Resource, cyanTex->UploadHeap));

    auto navyTex = std::make_unique<Texture>();
    navyTex->Name = "navyTex";
    navyTex->Filename = L"../../Textures/navyTex.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), navyTex->Filename.c_str(),
        navyTex->Resource, navyTex->UploadHeap));

    auto brownTex = std::make_unique<Texture>();
    brownTex->Name = "brownTex";
    brownTex->Filename = L"../../Textures/brownTex.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), brownTex->Filename.c_str(),
        brownTex->Resource, brownTex->UploadHeap));

    auto waterTex = std::make_unique<Texture>();
    waterTex->Name = "waterTex";
    waterTex->Filename = L"../../Textures/water3.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), waterTex->Filename.c_str(),
        waterTex->Resource, waterTex->UploadHeap));

    auto gateTex = std::make_unique<Texture>();
    gateTex->Name = "gateTex";
    gateTex->Filename = L"../../Textures/gate.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), gateTex->Filename.c_str(),
        gateTex->Resource, gateTex->UploadHeap));

    auto treeArrayTex = std::make_unique<Texture>();
    treeArrayTex->Name = "treeArrayTex";
    treeArrayTex->Filename = L"../../Textures/newTree.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), treeArrayTex->Filename.c_str(),
        treeArrayTex->Resource, treeArrayTex->UploadHeap));
    
    auto cloudArrayTex = std::make_unique<Texture>();
    cloudArrayTex->Name = "cloudArrayTex";
    cloudArrayTex->Filename = L"../../Textures/cloud0.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), cloudArrayTex->Filename.c_str(),
        cloudArrayTex->Resource, cloudArrayTex->UploadHeap));
    
    auto wyvernArrayTex = std::make_unique<Texture>();
    wyvernArrayTex->Name = "wyvernArrayTex";
    wyvernArrayTex->Filename = L"../../Textures/rathalos.dds";
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
        mCommandList.Get(), wyvernArrayTex->Filename.c_str(),
        wyvernArrayTex->Resource, wyvernArrayTex->UploadHeap));



    mTextures[bricksTex->Name] = std::move(bricksTex);
    mTextures[stoneTex->Name] = std::move(stoneTex);
    mTextures[tileTex->Name] = std::move(tileTex);
    mTextures[crateTex->Name] = std::move(crateTex);
    mTextures[iceTex->Name] = std::move(iceTex);
    mTextures[defaultTex->Name] = std::move(defaultTex);
    mTextures[coneTex->Name] = std::move(coneTex);
    mTextures[cylinderTex->Name] = std::move(cylinderTex);
    mTextures[innerBoxTex->Name] = std::move(innerBoxTex);
    mTextures[outerBoxTex->Name] = std::move(outerBoxTex);
    mTextures[diamondTex->Name] = std::move(diamondTex);
    mTextures[cutPyramidTex->Name] = std::move(cutPyramidTex);
    mTextures[holoTex->Name] = std::move(holoTex);
    mTextures[redTex->Name] = std::move(redTex);
    mTextures[cyanTex->Name] = std::move(cyanTex);
    mTextures[navyTex->Name] = std::move(navyTex);
    mTextures[brownTex->Name] = std::move(brownTex);
    mTextures[waterTex->Name] = std::move(waterTex);
    mTextures[gateTex->Name] = std::move(gateTex);
    mTextures[treeArrayTex->Name] = std::move(treeArrayTex);
    mTextures[cloudArrayTex->Name] = std::move(cloudArrayTex);
    mTextures[wyvernArrayTex->Name] = std::move(wyvernArrayTex);


    ::OutputDebugStringA(">>> LoadTextures DONE!\n");
}



//assuming we have n renter items, we can populate the CBV heap with the following code where descriptors 0 to n-
//1 contain the object CBVs for the 0th frame resource, descriptors n to 2n−1 contains the
//object CBVs for 1st frame resource, descriptors 2n to 3n−1 contain the objects CBVs for
//the 2nd frame resource, and descriptors 3n, 3n + 1, and 3n + 2 contain the pass CBVs for the
//0th, 1st, and 2nd frame resource
void ShapesApp::BuildConstantBufferViews()
{
    ::OutputDebugStringA(">>> BuildConstantBufferViews started...\n");

    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    UINT objCount = (UINT)mRitemLayer[(int)RenderLayer::Count].size();

    // Need a CBV descriptor for each object for each frame resource.
    for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
    {
        auto objectCB = mFrameResources[frameIndex]->ObjectCB->Resource();
        for (UINT i = 0; i < objCount; ++i)
        {
            D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();

            // Offset to the ith object constant buffer in the buffer.
            cbAddress += i * objCBByteSize;

            // Offset to the object cbv in the descriptor heap.
            int heapIndex = frameIndex * objCount + i;

            //we can get a handle to the first descriptor in a heap with the ID3D12DescriptorHeap::GetCPUDescriptorHandleForHeapStart
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());

            //our heap has more than one descriptor,we need to know the size to increment in the heap to get to the next descriptor
            //This is hardware specific, so we have to query this information from the device, and it depends on
            //the heap type.Recall that our D3DApp class caches this information: 	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
            cbvDesc.BufferLocation = cbAddress;
            cbvDesc.SizeInBytes = objCBByteSize;

            md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
        }
    }

    UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

    // Last three descriptors are the pass CBVs for each frame resource.
    for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
    {
        auto passCB = mFrameResources[frameIndex]->PassCB->Resource();
        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

        // Offset to the pass cbv in the descriptor heap.
        int heapIndex = mPassCbvOffset + frameIndex;
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = cbAddress;
        cbvDesc.SizeInBytes = passCBByteSize;

        md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
    }

    ::OutputDebugStringA(">>> BuildConstantBufferViews DONE!\n");
}

//A root signature defines what resources need to be bound to the pipeline before issuing a draw call and
//how those resources get mapped to shader input registers. there is a limit of 64 DWORDs that can be put in a root signature.
void ShapesApp::BuildRootSignature()
{
    ::OutputDebugStringA(">>> BuildRootSignature started...\n");

    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        1,  // number of descriptors
        0); // register t0

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[5]; //EDIT

    // Perfomance TIP: Order from most frequent to least frequent.
    slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[1].InitAsConstantBufferView(0); // register b0
    slotRootParameter[2].InitAsConstantBufferView(1); // register b1
    slotRootParameter[3].InitAsConstantBufferView(2); // register b2
    slotRootParameter[4].InitAsConstantBufferView(3); // register b3 //EDIT

    auto staticSamplers = GetStaticSamplers();

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter,
        (UINT)staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(mRootSignature.GetAddressOf())));

    ::OutputDebugStringA(">>> BuildRootSignature DONE!\n");
}

//If we have 3 frame resources and n render items, then we have three 3n object constant
//buffers and 3 pass constant buffers.Hence we need 3(n + 1) constant buffer views(CBVs).
//Thus we will need to modify our CBV heap to include the additional descriptors :
void ShapesApp::BuildDescriptorHeaps()
{
    ::OutputDebugStringA(">>> BuildDescriptorHeaps started...\n");

    //
    // Create the SRV heap.
    //
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 22; //EDIT NUM OF DESCRIPTORS
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

    //
    // Fill out the heap with actual descriptors.
    //
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    auto bricksTex = mTextures["bricksTex"]->Resource;
    auto stoneTex = mTextures["stoneTex"]->Resource;
    auto tileTex = mTextures["tileTex"]->Resource;
    auto coneTex = mTextures["coneTex"]->Resource;
    auto cylinderTex = mTextures["cylinderTex"]->Resource;
    auto innerBoxTex = mTextures["innerBoxTex"]->Resource;
    auto outerBoxTex = mTextures["outerBoxTex"]->Resource;
    auto diamondTex = mTextures["diamondTex"]->Resource;
    auto cutPyramidTex = mTextures["cutPyramidTex"]->Resource;
    auto holoTex = mTextures["holoTex"]->Resource;
    auto redTex = mTextures["redTex"]->Resource;
    auto cyanTex = mTextures["cyanTex"]->Resource;
    auto navyTex = mTextures["navyTex"]->Resource;
    auto brownTex = mTextures["brownTex"]->Resource;
    auto waterTex = mTextures["waterTex"]->Resource;
    auto treeArrayTex = mTextures["treeArrayTex"]->Resource;
    auto cloudArrayTex = mTextures["cloudArrayTex"]->Resource;
    auto wyvernArrayTex = mTextures["wyvernArrayTex"]->Resource;
    auto gateTex = mTextures["gateTex"]->Resource;


    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = bricksTex->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = bricksTex->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    md3dDevice->CreateShaderResourceView(bricksTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);

    srvDesc.Format = stoneTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = stoneTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(stoneTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.Format = tileTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = tileTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(tileTex.Get(), &srvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.Format = coneTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = coneTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(coneTex.Get(), &srvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.Format = cylinderTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = cylinderTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(cylinderTex.Get(), &srvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.Format = innerBoxTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = innerBoxTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(innerBoxTex.Get(), &srvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.Format = outerBoxTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = outerBoxTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(outerBoxTex.Get(), &srvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.Format = diamondTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = diamondTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(diamondTex.Get(), &srvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.Format = cutPyramidTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = cutPyramidTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(cutPyramidTex.Get(), &srvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.Format = holoTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = holoTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(holoTex.Get(), &srvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.Format = redTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = redTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(redTex.Get(), &srvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.Format = cyanTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = cyanTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(cyanTex.Get(), &srvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.Format = navyTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = navyTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(navyTex.Get(), &srvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.Format = brownTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = brownTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(brownTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);

    srvDesc.Format = waterTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = waterTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDescriptor);

    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.Format = gateTex->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = gateTex->GetDesc().MipLevels;
    md3dDevice->CreateShaderResourceView(gateTex.Get(), &srvDesc, hDescriptor);

    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);

    auto desc = treeArrayTex->GetDesc();
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Format = treeArrayTex->GetDesc().Format;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = -1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = treeArrayTex->GetDesc().DepthOrArraySize;
    md3dDevice->CreateShaderResourceView(treeArrayTex.Get(), &srvDesc, hDescriptor);
    
    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Format = cloudArrayTex->GetDesc().Format;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = -1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = cloudArrayTex->GetDesc().DepthOrArraySize;
    md3dDevice->CreateShaderResourceView(cloudArrayTex.Get(), &srvDesc, hDescriptor);
    
    // next descriptor
    hDescriptor.Offset(1, mCbvSrvDescriptorSize);
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Format = wyvernArrayTex->GetDesc().Format;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = -1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = wyvernArrayTex->GetDesc().DepthOrArraySize;
    md3dDevice->CreateShaderResourceView(wyvernArrayTex.Get(), &srvDesc, hDescriptor);

    ::OutputDebugStringA(">>> BuildDescriptorHeaps DONE!\n");
}

void ShapesApp::BuildShadersAndInputLayout()
{
    ::OutputDebugStringA(">>> BuildShadersAndInputLayout started...\n");

    const D3D_SHADER_MACRO defines[] =
    {
        "FOG", "1",
        NULL, NULL
    };

    const D3D_SHADER_MACRO alphaTestDefines[] =
    {
        "FOG", "1",
        "ALPHA_TEST", "1",
        NULL, NULL
    };

    mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
    mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");

    mShaders["treeSpriteVS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "VS", "vs_5_1");
    mShaders["treeSpriteGS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "GS", "gs_5_1");
    mShaders["treeSpritePS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_1");

    mStdInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    mTreeSpriteInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    ////When creating an input layout, you can specify that data streams in per - instance rather than at a per - vertex frequency by using D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
    ////You would then bind a secondary vertex buffer to the input stream that contained the instancing data.
    ////The modern approach is to create a structured buffer that contains the per - instance data for all of our instances.

    //mInputLayout =
    //{
    //    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //};

    ::OutputDebugStringA(">>> BuildShadersAndInputLayout DONE!\n");
}

void ShapesApp::BuildSkullGeometry()
{
    std::ifstream fin("Models/skull.txt");

    if (!fin)
    {
        MessageBox(0, L"Models/skull.txt not found.", 0, 0);
        return;
    }

    UINT vcount = 0;
    UINT tcount = 0;
    std::string ignore;

    fin >> ignore >> vcount;
    fin >> ignore >> tcount;
    fin >> ignore >> ignore >> ignore >> ignore;

    std::vector<Vertex> vertices(vcount);
    for (UINT i = 0; i < vcount; ++i)
    {
        fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
        fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

        XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);

        // Project point onto unit sphere and generate spherical texture coordinates.
        XMFLOAT3 spherePos;
        XMStoreFloat3(&spherePos, XMVector3Normalize(P));

        float theta = atan2f(spherePos.z, spherePos.x);

        // Put in [0, 2pi].
        if (theta < 0.0f)
            theta += XM_2PI;

        float phi = acosf(spherePos.y);

        float u = theta / (2.0f * XM_PI);
        float v = phi / XM_PI;

        vertices[i].TexC = { u, v };

    }


    fin >> ignore;
    fin >> ignore;
    fin >> ignore;

    std::vector<std::int32_t> indices(3 * tcount);
    for (UINT i = 0; i < tcount; ++i)
    {
        fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
    }

    fin.close();

    //
    // Pack the indices of all the meshes into one index buffer.
    //

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::int32_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "skullGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R32_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs["skull"] = submesh;

    mGeometries[geo->Name] = std::move(geo);
}

void ShapesApp::BuildLandGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(300.0f, 300.0f, 50, 50);

    //
    // Extract the vertex elements we are interested and apply the height function to
    // each vertex.  In addition, color the vertices based on their height so we have
    // sandy looking beaches, grassy low hills, and snow mountain peaks.
    //

    std::vector<Vertex> vertices(grid.Vertices.size());
    for (size_t i = 0; i < grid.Vertices.size(); ++i)
    {
        auto& p = grid.Vertices[i].Position;
        vertices[i].Pos = p;
        vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
        vertices[i].Normal = GetHillsNormal(p.x, p.z);
        vertices[i].TexC = grid.Vertices[i].TexC;
    }

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

    std::vector<std::uint16_t> indices = grid.GetIndices16();
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "landGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs["grid"] = submesh;

    mGeometries["landGeo"] = std::move(geo);
}

void ShapesApp::BuildWavesGeometry()
{
    ::OutputDebugStringA(">>> BuildWavesGeometry started...\n");

    std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
    assert(mWaves->VertexCount() < 0x0000ffff);

    // Iterate over each quad.
    int m = mWaves->RowCount();
    int n = mWaves->ColumnCount();
    int k = 0;
    for (int i = 0; i < m - 1; ++i)
    {
        for (int j = 0; j < n - 1; ++j)
        {
            indices[k] = i * n + j;
            indices[k + 1] = i * n + j + 1;
            indices[k + 2] = (i + 1) * n + j;

            indices[k + 3] = (i + 1) * n + j;
            indices[k + 4] = i * n + j + 1;
            indices[k + 5] = (i + 1) * n + j + 1;

            k += 6; // next quad
        }
    }

    UINT vbByteSize = mWaves->VertexCount() * sizeof(Vertex);
    UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "waterGeo";

    // Set dynamically.
    geo->VertexBufferCPU = nullptr;
    geo->VertexBufferGPU = nullptr;

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs["grid"] = submesh;

    mGeometries["waterGeo"] = std::move(geo);

    ::OutputDebugStringA(">>> BuildWavesGeometry DONE!\n");
}


void ShapesApp::BuildOneShapeGeometry(std::string shape_type, std::string shape_name, float param_a, float param_b, float param_c, float param_d, float param_e) {
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData mesh;
    BoundingBox bounds;

    if (shape_type == "box" ||
        shape_type == "outterWall" ||
        shape_type == "tower" ||
        shape_type == "gate") {
        mesh = geoGen.CreateBox(param_a, param_b, param_c, param_d);
        if (shape_type == "box") {
            XMStoreFloat3(&bounds.Center, XMVectorSet(0, 0, 0, 1.0f));
            XMStoreFloat3(&bounds.Extents, 0.5f * XMVectorSet(param_a, param_b, param_c, 1.0f));
        }
    }
    if (shape_type == "grid") {
        mesh = geoGen.CreateGrid(param_a, param_b, param_c, param_d);
    }
    if (shape_type == "sphere") {
        mesh = geoGen.CreateSphere(param_a, param_b, param_c);
    }
    if (shape_type == "cylinder" ||
        shape_type == "rolo") {
        mesh = geoGen.CreateCylinder(param_a, param_b, param_c, param_d, param_e);
    }
    if (shape_type == "wedge") {
        mesh = geoGen.CreateWedge(param_a, param_b, param_c, param_d);
    }
    if (shape_type == "cone") {
        mesh = geoGen.CreateCone(param_a, param_b, param_c, param_d);
    }
    if (shape_type == "pyramid") {
        mesh = geoGen.CreatePyramid(param_a, param_b, param_c);
    }
    if (shape_type == "truncatedPyramid") {
        mesh = geoGen.CreateTruncatedPyramid(param_a, param_b, param_c, param_d);
    }
    if (shape_type == "diamond" ||
        shape_type == "charm") {
        mesh = geoGen.CreateDiamond(param_a, param_b, param_c, param_d);
    }
    if (shape_type == "prism") {
        mesh = geoGen.CreateTriangularPrism(param_a, param_b, param_c);
    }
    if (shape_type == "torus") {
        mesh = geoGen.CreateTorus(param_a, param_b, param_c, param_d);
    }


    std::vector<Vertex> vertices(mesh.Vertices.size());
    for (size_t i = 0; i < mesh.Vertices.size(); ++i)
    {
        auto& p = mesh.Vertices[i].Position;
        vertices[i].Pos = p;
        vertices[i].Normal = mesh.Vertices[i].Normal;
        vertices[i].TexC = mesh.Vertices[i].TexC;
    }

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

    std::vector<std::uint16_t> indices = mesh.GetIndices16();
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = shape_name;

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    submesh.Bounds = bounds;

    geo->DrawArgs[shape_type] = submesh;

    mGeometries[shape_name] = std::move(geo);
}

void ShapesApp::BuildShapeGeometry()
{
    ::OutputDebugStringA(">>> BuildShapeGeometry started...\n");

    BuildOneShapeGeometry("box", "boxGeo", 1.0f, 1.0f, 1.0f, 3);
    BuildOneShapeGeometry("outterWall", "outterWallGeo", 1.0f, 1.0f, 1.0f, 3);
    BuildOneShapeGeometry("tower", "towerGeo", 1.0f, 1.0f, 1.0f, 3);
    BuildOneShapeGeometry("gate", "gateGeo", 1.0f, 1.0f, 1.0f, 3);
    BuildOneShapeGeometry("grid", "gridGeo", 70.0f, 70.0f, 60, 40);
    BuildOneShapeGeometry("sphere", "sphereGeo", 0.5f, 20, 20);
    BuildOneShapeGeometry("cylinder", "cylinderGeo", 1.0f, 1.0f, 2.0f, 20, 20);
    BuildOneShapeGeometry("rolo", "roloGeo", 1.0f, 0.5f, 1.0f, 20, 20);
    BuildOneShapeGeometry("wedge", "wedgeGeo", 1.0, 1.0f, 1.0, 3);
    BuildOneShapeGeometry("cone", "coneGeo", 1.0f, 2.0f, 20, 20);
    BuildOneShapeGeometry("pyramid", "pyramidGeo", 1.0, 1.0f, 20);
    BuildOneShapeGeometry("truncatedPyramid", "truncatedPyramidGeo", 1.0, 1.0f, 0.5f, 1);
    BuildOneShapeGeometry("diamond", "diamondGeo", 1.0f, 1.0f, 1.0f, 1);
    BuildOneShapeGeometry("charm", "charmGeo", 1.0f, 1.0f, 1.0f, 1);
    BuildOneShapeGeometry("prism", "prismGeo", 1.0f, 1.0f, 1);
    BuildOneShapeGeometry("torus", "torusGeo", 2.0f, 0.5f, 20, 20);

    ::OutputDebugStringA(">>> BuildShapeGeometry DONE!\n");
}

void ShapesApp::BuildTreeSpritesGeometry()
{
    //step5
    struct TreeSpriteVertex
    {
        XMFLOAT3 Pos;
        XMFLOAT2 Size;
    };

    static const int treeCount = 16;
    std::array<TreeSpriteVertex, 16> vertices;
    for (UINT i = 0; i < treeCount; ++i)
    {
        float x = MathHelper::RandF(-100.0f, 100.0f);
        float z = MathHelper::RandF(-100.0f, 100.0f);
        float y = GetHillsHeight(x, z);

        // Move tree slightly above land height.
        y += 8.0f;

        vertices[i].Pos = XMFLOAT3(x, y, z);
        vertices[i].Size = XMFLOAT2(30.0f, 35.0f);
    }

    std::array<std::uint16_t, 16> indices =
    {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15
    };

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "treeSpritesGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(TreeSpriteVertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs["points"] = submesh;

    mGeometries["treeSpritesGeo"] = std::move(geo);
}

void ShapesApp::BuildCloudSpritesGeometry()
{
    //step5
    struct TreeSpriteVertex
    {
        XMFLOAT3 Pos;
        XMFLOAT2 Size;
    };

    static const int treeCount = 16;
    std::array<TreeSpriteVertex, 16> vertices;
    for (UINT i = 0; i < treeCount; ++i)
    {
        
        float x = MathHelper::RandF(-150.0f, 150.0f);
        float z = 0;
        if (i < treeCount * 0.5) {
            z = MathHelper::RandF(50.0f, 150.0f);
        }
        else {
            z = MathHelper::RandF(-50.0f, -150.0f);
        }

        float y = 90.0f;


        vertices[i].Pos = XMFLOAT3(x, y, z);
        vertices[i].Size = XMFLOAT2(48, 20);
    }

    std::array<std::uint16_t, 16> indices =
    {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15
    };

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "cloudSpritesGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(TreeSpriteVertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs["points"] = submesh;

    mGeometries["cloudSpritesGeo"] = std::move(geo);
}

void ShapesApp::BuildWyvernSpritesGeometry()
{
    //step5
    struct TreeSpriteVertex
    {
        XMFLOAT3 Pos;
        XMFLOAT2 Size;
    };

    static const int treeCount = 1;
    std::array<TreeSpriteVertex, 1> vertices;
    for (UINT i = 0; i < treeCount; ++i)
    {
        vertices[i].Pos = XMFLOAT3(0, 75, 150);
        vertices[i].Size = XMFLOAT2(100, 100);
    }

    std::array<std::uint16_t, 1> indices =
    {
        0
    };

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "wyvernSpritesGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(TreeSpriteVertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT; //can't be DXGI_FORMAT_R8_UINT due to D3D12 ERROR: ID3D12CommandList::IASetIndexBuffer: The Format (0x3e, R8_UINT) is not valid for usage as an index buffer format. [ EXECUTION ERROR #715: SET_INDEX_BUFFER_INVALID_DESC]
                                            // and D3D12 ERROR : ID3D12CommandList::DrawIndexedInstanced : The current index buffer Format(0x3e, R8_UINT) is not valid for usage as an index buffer Format.[EXECUTION ERROR #212: COMMAND_LIST_DRAW_INDEX_BUFFER_FORMAT_INVALID]
    geo->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;

    geo->DrawArgs["points"] = submesh;

    mGeometries["wyvernSpritesGeo"] = std::move(geo);
}

void ShapesApp::BuildPSOs()
{
    ::OutputDebugStringA(">>> BuildPSOs started...\n");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

    //
    // PSO for opaque objects.
    //
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    opaquePsoDesc.InputLayout = { mStdInputLayout.data(), (UINT)mStdInputLayout.size() };
    //opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    opaquePsoDesc.pRootSignature = mRootSignature.Get();
    opaquePsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
        mShaders["standardVS"]->GetBufferSize()
    };
    opaquePsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
        mShaders["opaquePS"]->GetBufferSize()
    };
    opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    //opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    opaquePsoDesc.SampleMask = UINT_MAX;
    opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    opaquePsoDesc.NumRenderTargets = 1;
    opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
    opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    opaquePsoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));


    //
    // PSO for transparent objects
    //

    D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

    D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
    transparencyBlendDesc.BlendEnable = true;
    transparencyBlendDesc.LogicOpEnable = false;
    transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    //transparentPsoDesc.BlendState.AlphaToCoverageEnable = true;

    transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

    //
    // PSO for alpha tested objects
    //

    D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
    alphaTestedPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
        mShaders["alphaTestedPS"]->GetBufferSize()
    };
    alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));

    //
    // PSO for tree sprites
    //
    D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
    treeSpritePsoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["treeSpriteVS"]->GetBufferPointer()),
        mShaders["treeSpriteVS"]->GetBufferSize()
    };
    treeSpritePsoDesc.GS =
    {
        reinterpret_cast<BYTE*>(mShaders["treeSpriteGS"]->GetBufferPointer()),
        mShaders["treeSpriteGS"]->GetBufferSize()
    };
    treeSpritePsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["treeSpritePS"]->GetBufferPointer()),
        mShaders["treeSpritePS"]->GetBufferSize()
    };
    //step1
    treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
    treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs["treeSprites"])));

    //
    // PSO for opaque wireframe objects.
    //
    //D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
    //opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    //ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));

    ::OutputDebugStringA(">>> BuildPSOs DONE!\n");
}

void ShapesApp::BuildFrameResources()
{
    ::OutputDebugStringA(">>> BuildFrameResources started...\n");

    for (int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
            1, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
    }

    ::OutputDebugStringA(">>> BuildFrameResources DONE!\n");
}

void ShapesApp::BuildMaterials() //EDIT MATS HERE
{
    ::OutputDebugStringA(">>> BuildMaterials started...\n");

    int mat_idx = 0;

    auto bricks0 = std::make_unique<Material>();
    bricks0->Name = "bricks0";
    bricks0->MatCBIndex = mat_idx;
    bricks0->DiffuseSrvHeapIndex = mat_idx;
    bricks0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    bricks0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    bricks0->Roughness = 0.1f;
    mat_idx++;

    auto stone0 = std::make_unique<Material>();
    stone0->Name = "stone0";
    stone0->MatCBIndex = mat_idx;
    stone0->DiffuseSrvHeapIndex = mat_idx;
    stone0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    stone0->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    stone0->Roughness = 0.3f;
    mat_idx++;

    auto tile0 = std::make_unique<Material>();
    tile0->Name = "tile0";
    tile0->MatCBIndex = mat_idx;
    tile0->DiffuseSrvHeapIndex = mat_idx;
    tile0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    tile0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    tile0->Roughness = 0.3f;
    mat_idx++;

    auto cone0 = std::make_unique<Material>();
    cone0->Name = "cone0";
    cone0->MatCBIndex = mat_idx;
    cone0->DiffuseSrvHeapIndex = mat_idx;
    cone0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    cone0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    cone0->Roughness = 0.3f;
    mat_idx++;

    auto cylinder0 = std::make_unique<Material>();
    cylinder0->Name = "cylinder0";
    cylinder0->MatCBIndex = mat_idx;
    cylinder0->DiffuseSrvHeapIndex = mat_idx;
    cylinder0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    cylinder0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    cylinder0->Roughness = 0.3f;
    mat_idx++;

    auto inner0 = std::make_unique<Material>();
    inner0->Name = "inner0";
    inner0->MatCBIndex = mat_idx;
    inner0->DiffuseSrvHeapIndex = mat_idx;
    inner0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    inner0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    inner0->Roughness = 0.3f;
    mat_idx++;

    auto outer0 = std::make_unique<Material>();
    outer0->Name = "outer0";
    outer0->MatCBIndex = mat_idx;
    outer0->DiffuseSrvHeapIndex = mat_idx;
    outer0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    outer0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    outer0->Roughness = 0.3f;
    mat_idx++;

    auto diamond0 = std::make_unique<Material>();
    diamond0->Name = "diamond0";
    diamond0->MatCBIndex = mat_idx;
    diamond0->DiffuseSrvHeapIndex = mat_idx;
    diamond0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    diamond0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    diamond0->Roughness = 0.3f;
    mat_idx++;

    auto cutPyr0 = std::make_unique<Material>();
    cutPyr0->Name = "cutPyr0";
    cutPyr0->MatCBIndex = mat_idx;
    cutPyr0->DiffuseSrvHeapIndex = mat_idx;
    cutPyr0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    cutPyr0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    cutPyr0->Roughness = 0.3f;
    mat_idx++;

    auto holo0 = std::make_unique<Material>();
    holo0->Name = "holo0";
    holo0->MatCBIndex = mat_idx;
    holo0->DiffuseSrvHeapIndex = mat_idx;
    holo0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    holo0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    holo0->Roughness = 0.3f;
    mat_idx++;

    auto red0 = std::make_unique<Material>();
    red0->Name = "red0";
    red0->MatCBIndex = mat_idx;
    red0->DiffuseSrvHeapIndex = mat_idx;
    red0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    red0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    red0->Roughness = 0.3f;
    mat_idx++;

    auto cyan0 = std::make_unique<Material>();
    cyan0->Name = "cyan0";
    cyan0->MatCBIndex = mat_idx;
    cyan0->DiffuseSrvHeapIndex = mat_idx;
    cyan0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    cyan0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    cyan0->Roughness = 0.3f;
    mat_idx++;

    auto navy0 = std::make_unique<Material>();
    navy0->Name = "navy0";
    navy0->MatCBIndex = mat_idx;
    navy0->DiffuseSrvHeapIndex = mat_idx;
    navy0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    navy0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    navy0->Roughness = 0.3f;
    mat_idx++;

    auto brown0 = std::make_unique<Material>();
    brown0->Name = "brown0";
    brown0->MatCBIndex = mat_idx;
    brown0->DiffuseSrvHeapIndex = mat_idx;
    brown0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    brown0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    brown0->Roughness = 0.3f;
    mat_idx++;

    // This is not a good water material definition, but we do not have all the rendering
    // tools we need (transparency, environment reflection), so we fake it for now.
    auto water = std::make_unique<Material>();
    water->Name = "water";
    water->MatCBIndex = mat_idx; //EDIT
    water->DiffuseSrvHeapIndex = mat_idx; //EDIT
    water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.4f);
    water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    water->Roughness = 0.0f;
    mat_idx++;

    auto gate0 = std::make_unique<Material>();
    gate0->Name = "gate0";
    gate0->MatCBIndex = mat_idx;
    gate0->DiffuseSrvHeapIndex = mat_idx;
    gate0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
    gate0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    gate0->Roughness = 0.3f;
    mat_idx++;


    auto treeSprites = std::make_unique<Material>();
    treeSprites->Name = "treeSprites";
    treeSprites->MatCBIndex = mat_idx;
    treeSprites->DiffuseSrvHeapIndex = mat_idx;
    treeSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    treeSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
    treeSprites->Roughness = 0.125f;
    mat_idx++;
    
    auto cloudSprites = std::make_unique<Material>();
    cloudSprites->Name = "cloudSprites";
    cloudSprites->MatCBIndex = mat_idx;
    cloudSprites->DiffuseSrvHeapIndex = mat_idx;
    cloudSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    cloudSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
    cloudSprites->Roughness = 0.125f;
    mat_idx++;
    
    auto wyvernSprites = std::make_unique<Material>();
    wyvernSprites->Name = "wyvernSprites";
    wyvernSprites->MatCBIndex = mat_idx;
    wyvernSprites->DiffuseSrvHeapIndex = mat_idx;
    wyvernSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    wyvernSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
    wyvernSprites->Roughness = 0.125f;
    mat_idx++;

    mMaterials["bricks0"] = std::move(bricks0);
    mMaterials["stone0"] = std::move(stone0);
    mMaterials["tile0"] = std::move(tile0);
    mMaterials["cone0"] = std::move(cone0);
    mMaterials["cylinder0"] = std::move(cylinder0);
    mMaterials["inner0"] = std::move(inner0);
    mMaterials["outer0"] = std::move(outer0);
    mMaterials["diamond0"] = std::move(diamond0);
    mMaterials["cutPyr0"] = std::move(cutPyr0);
    mMaterials["holo0"] = std::move(holo0);
    mMaterials["red0"] = std::move(red0);
    mMaterials["cyan0"] = std::move(cyan0);
    mMaterials["navy0"] = std::move(navy0);
    mMaterials["brown0"] = std::move(brown0);
    mMaterials["water"] = std::move(water);
    mMaterials["gate0"] = std::move(gate0);
    mMaterials["treeSprites"] = std::move(treeSprites);
    mMaterials["cloudSprites"] = std::move(cloudSprites);
    mMaterials["wyvernSprites"] = std::move(wyvernSprites);

    ::OutputDebugStringA(">>> BuildMaterials DONE!\n");
}


void ShapesApp::BuildOneRenderItem(std::string shape_type, std::string shape_name, std::string material, XMMATRIX scale_matrix, XMMATRIX translate_matrix, XMMATRIX tex_scale_matrix, UINT obj_idx)
{
    auto shape_render_item = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&shape_render_item->World, scale_matrix * translate_matrix);
    XMStoreFloat4x4(&shape_render_item->TexTransform, tex_scale_matrix);
    shape_render_item->ObjCBIndex = obj_idx;
    shape_render_item->Mat = mMaterials[material].get();
    shape_render_item->Geo = mGeometries[shape_name].get();
    shape_render_item->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    shape_render_item->IndexCount = shape_render_item->Geo->DrawArgs[shape_type].IndexCount;
    shape_render_item->StartIndexLocation = shape_render_item->Geo->DrawArgs[shape_type].StartIndexLocation;
    shape_render_item->BaseVertexLocation = shape_render_item->Geo->DrawArgs[shape_type].BaseVertexLocation;
    if (shape_type == "box") {
        BoundingBox bounds;
        XMStoreFloat3(&bounds.Center, XMVectorSet(XMVectorGetX(translate_matrix.r[3]), XMVectorGetY(translate_matrix.r[3]), XMVectorGetZ(translate_matrix.r[3]), 1.0f));
        XMStoreFloat3(&bounds.Extents, 0.5f * XMVectorSet(XMVectorGetX(scale_matrix.r[0]), XMVectorGetY(scale_matrix.r[1]), XMVectorGetZ(scale_matrix.r[2]), 1.0f));

        shape_render_item->Bounds = bounds;
    }
    else {
        shape_render_item->Bounds = shape_render_item->Geo->DrawArgs[shape_type].Bounds;
    }
    mRitemLayer[(int)RenderLayer::Opaque].push_back(shape_render_item.get());
    mAllRitems.push_back(std::move(shape_render_item));
}

void ShapesApp::BuildOneRenderItem(std::string shape_type, std::string shape_name, std::string material, XMMATRIX rotate_matrix, XMMATRIX scale_matrix, XMMATRIX translate_matrix, XMMATRIX tex_scale_matrix, UINT obj_idx)
{
    auto shape_render_item = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&shape_render_item->World, scale_matrix * rotate_matrix * translate_matrix); //difference between this and func above
    XMStoreFloat4x4(&shape_render_item->TexTransform, tex_scale_matrix);
    shape_render_item->ObjCBIndex = obj_idx;
    shape_render_item->Mat = mMaterials[material].get();
    shape_render_item->Geo = mGeometries[shape_name].get();
    shape_render_item->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    shape_render_item->IndexCount = shape_render_item->Geo->DrawArgs[shape_type].IndexCount;
    shape_render_item->StartIndexLocation = shape_render_item->Geo->DrawArgs[shape_type].StartIndexLocation;
    shape_render_item->BaseVertexLocation = shape_render_item->Geo->DrawArgs[shape_type].BaseVertexLocation;
    shape_render_item->Bounds = shape_render_item->Geo->DrawArgs[shape_type].Bounds;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(shape_render_item.get());
    mAllRitems.push_back(std::move(shape_render_item));
}


//void ShapesApp::BuildRenderItems()
//{
//    /*auto boxRitem = std::make_unique<RenderItem>();
//    XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(0.0f, 0.5f, 0.0f));
//    boxRitem->ObjCBIndex = 0;
//    boxRitem->Geo = mGeometries["shapeGeo"].get();
//    boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//    boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
//    boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
//    boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
//    mAllRitems.push_back(std::move(boxRitem));*/
//
//    BuildOneRenderItem("box", XMMatrixScaling(2.0f, 2.0f, 2.0f), XMMatrixTranslation(0.0f, 0.5f, 0.0f), 0);
//    BuildOneRenderItem("diamond", XMMatrixScaling(1.0f, 1.0f, 1.0f), XMMatrixTranslation(0.0f, 0.0f, 0.0f), 1);
//    BuildOneRenderItem("prism", XMMatrixScaling(2.0f, 2.0f, 2.0f), XMMatrixTranslation(0.0f, 0.5f, 0.0f), 2);
//
//    auto gridRitem = std::make_unique<RenderItem>();
//    gridRitem->World = MathHelper::Identity4x4();
//    gridRitem->ObjCBIndex = 3;
//    gridRitem->Geo = mGeometries["shapeGeo"].get();
//    gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
//    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
//    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
//    mAllRitems.push_back(std::move(gridRitem));
//
//    UINT objCBIndex = 4;
//    for (int i = 0; i < 5; ++i)
//    {
//        auto leftCylRitem = std::make_unique<RenderItem>();
//        auto rightCylRitem = std::make_unique<RenderItem>();
//        auto leftSphereRitem = std::make_unique<RenderItem>();
//        auto rightSphereRitem = std::make_unique<RenderItem>();
//
//        XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
//        XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);
//
//        XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
//        XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);
//
//        XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
//        leftCylRitem->ObjCBIndex = objCBIndex++;
//        leftCylRitem->Geo = mGeometries["shapeGeo"].get();
//        leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//        leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
//        leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
//        leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
//
//        XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
//        rightCylRitem->ObjCBIndex = objCBIndex++;
//        rightCylRitem->Geo = mGeometries["shapeGeo"].get();
//        rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//        rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
//        rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
//        rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
//
//        XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
//        leftSphereRitem->ObjCBIndex = objCBIndex++;
//        leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
//        leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//        leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
//        leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
//        leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
//
//        XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
//        rightSphereRitem->ObjCBIndex = objCBIndex++;
//        rightSphereRitem->Geo = mGeometries["shapeGeo"].get();
//        rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//        rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
//        rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
//        rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
//
//        mAllRitems.push_back(std::move(leftCylRitem));
//        mAllRitems.push_back(std::move(rightCylRitem));
//        mAllRitems.push_back(std::move(leftSphereRitem));
//        mAllRitems.push_back(std::move(rightSphereRitem));
//    }
//}

void ShapesApp::BuildRenderItems()
{
    ::OutputDebugStringA(">>> BuildRenderItems started...\n");

    UINT index_cache = 0;

    // waves
    auto wavesRitem = std::make_unique<RenderItem>();
    //wavesRitem->World = MathHelper::Identity4x4();
    XMStoreFloat4x4(&wavesRitem->World, XMMatrixScaling(1, 1, 1) * XMMatrixTranslation(0.0f, -10, 0));
    XMStoreFloat4x4(&wavesRitem->TexTransform, XMMatrixScaling(5, 5, 1.0f));
    wavesRitem->ObjCBIndex = index_cache;
    wavesRitem->Mat = mMaterials["water"].get();
    wavesRitem->Geo = mGeometries["waterGeo"].get();
    wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
    wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
    wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
    index_cache++;

    //// we use mVavesRitem in updatewaves() to set the dynamic VB of the wave renderitem to the current frame VB.
    mWavesRitem = wavesRitem.get();
    mRitemLayer[(int)RenderLayer::Transparent].push_back(wavesRitem.get());
    mAllRitems.push_back(std::move(wavesRitem)); //EXTREME MEGA IMPORTANT LINE

    // HILLS
    auto gridRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&gridRitem->World, XMMatrixScaling(1, 1, 1) * XMMatrixTranslation(0.0f, -5, 0));
    XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
    gridRitem->ObjCBIndex = index_cache;
    gridRitem->Mat = mMaterials["tile0"].get();
    gridRitem->Geo = mGeometries["landGeo"].get();
    gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
    index_cache++;

    mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
    mAllRitems.push_back(std::move(gridRitem));

    // base
    BuildOneRenderItem("truncatedPyramid", "truncatedPyramidGeo", "tile0", XMMatrixScaling(120, 20, 120), XMMatrixTranslation(0.0f, -10.1, 0.0f), XMMatrixScaling(1, 1, 1), index_cache++);


    // grid
    BuildOneRenderItem("grid", "gridGeo", "stone0", XMMatrixScaling(0.7f, 0.7f, 0.7f), XMMatrixTranslation(0.0f, 0.0f, 0.0f), XMMatrixScaling(0.7, 0.7, 1), index_cache++);
    
    
    // OUTTER
    // front wall 
    BuildOneRenderItem("outterWall", "outterWallGeo", "outer0", XMMatrixScaling(50.0f, 40, 4.0f), XMMatrixTranslation(0.0f, 19, -25.0f), XMMatrixScaling(1, 1, 1), index_cache++);
    // back wall
    BuildOneRenderItem("outterWall", "outterWallGeo", "outer0", XMMatrixScaling(50.0f, 40, 4.0f), XMMatrixTranslation(0.0f, 19, 25.0f), XMMatrixScaling(1, 1, 1), index_cache++);
    // left wall 
    BuildOneRenderItem("outterWall", "outterWallGeo", "outer0", XMMatrixRotationY(XMConvertToRadians(-90.0f)), XMMatrixScaling(50.0f, 40, 4.0f), XMMatrixTranslation(-25.0f, 19, 0.0f), XMMatrixScaling(1, 1, 1), index_cache++);
    // right wall
    BuildOneRenderItem("outterWall", "outterWallGeo", "outer0", XMMatrixRotationY(XMConvertToRadians(90.0f)), XMMatrixScaling(50.0f, 40, 4.0f), XMMatrixTranslation(25.0f, 19, 0.0f), XMMatrixScaling(1, 1, 1), index_cache++);

    const int kNumWallWedges = 12;
    // front wedge loop
    for (int i = 0; i < kNumWallWedges; ++i) {
        BuildOneRenderItem("wedge", "wedgeGeo", "cyan0", XMMatrixRotationY(XMConvertToRadians(180.0f)), XMMatrixScaling(2, 2, 2), XMMatrixTranslation(-20 + i * 4, 40, -26), XMMatrixScaling(1, 1, 1), index_cache++); //step = 4, min = -20
    }
    // back wedge loop
    for (int i = 0; i < kNumWallWedges; ++i) {
        BuildOneRenderItem("wedge", "wedgeGeo", "cyan0", XMMatrixScaling(2, 2, 2), XMMatrixTranslation(-20 + i * 4, 40, 26), XMMatrixScaling(1, 1, 1), index_cache++); //step = 4, min = -20
    }
    // left wedge loop
    for (int i = 0; i < kNumWallWedges; ++i) {
        BuildOneRenderItem("wedge", "wedgeGeo", "cyan0", XMMatrixRotationY(XMConvertToRadians(-90)), XMMatrixScaling(2, 2, 2), XMMatrixTranslation(-26, 40, -20 + i * 4), XMMatrixScaling(1, 1, 1), index_cache++); //step = 4, min = -20
    }
    // right wedge loop
    for (int i = 0; i < kNumWallWedges; ++i) {
        BuildOneRenderItem("wedge", "wedgeGeo", "cyan0", XMMatrixRotationY(XMConvertToRadians(90)), XMMatrixScaling(2, 2, 2), XMMatrixTranslation(26, 40, -20 + i * 4), XMMatrixScaling(1, 1, 1), index_cache++); //step = 4, min = -20
    }

    const int kNumWallPyramids = 6;
    // left pyramid loop
    for (int i = 0; i < kNumWallPyramids; ++i) {
        BuildOneRenderItem("truncatedPyramid", "truncatedPyramidGeo", "cutPyr0", XMMatrixRotationRollPitchYaw(XMConvertToRadians(0), XMConvertToRadians(0), XMConvertToRadians(90)), XMMatrixScaling(3, 2, 3), XMMatrixTranslation(-28, 34, -17.5 + i * 7), XMMatrixScaling(1, 1, 1), index_cache++); //step = 4, min = -20
    }
    // right pyramid loop
    for (int i = 0; i < kNumWallPyramids; ++i) {
        BuildOneRenderItem("truncatedPyramid", "truncatedPyramidGeo", "cutPyr0", XMMatrixRotationRollPitchYaw(XMConvertToRadians(0), XMConvertToRadians(0), XMConvertToRadians(-90)), XMMatrixScaling(3, 2, 3), XMMatrixTranslation(28, 34, -17.5 + i * 7), XMMatrixScaling(1, 1, 1), index_cache++); //step = 4, min = -20
    }
    // front rolo loop
    for (int i = 0; i < kNumWallPyramids; ++i) {
        BuildOneRenderItem("rolo", "roloGeo", "holo0", XMMatrixRotationRollPitchYaw(XMConvertToRadians(-90), XMConvertToRadians(0), XMConvertToRadians(0)), XMMatrixScaling(2, 2, 2), XMMatrixTranslation(-17.5 + i * 7, 34, -28), XMMatrixScaling(1, 1, 1), index_cache++); //step = 4, min = -20
    }
    // back rolo loop
    for (int i = 0; i < kNumWallPyramids; ++i) {
        BuildOneRenderItem("rolo", "roloGeo", "holo0", XMMatrixRotationRollPitchYaw(XMConvertToRadians(90), XMConvertToRadians(0), XMConvertToRadians(0)), XMMatrixScaling(2, 2, 2), XMMatrixTranslation(-17.5 + i * 7, 34, 28), XMMatrixScaling(1, 1, 1), index_cache++); //step = 4, min = -20
    }

    // outer towers
    for (int i = 0; i < 2; ++i)
    {
        // left towers
        BuildOneRenderItem("tower", "towerGeo", "navy0", XMMatrixScaling(8, 50, 8), XMMatrixTranslation(-25, 24, -25 + i * 50), XMMatrixScaling(1, 1, 1), index_cache++);
        //right towers
        BuildOneRenderItem("tower", "towerGeo", "navy0", XMMatrixScaling(8, 50, 8), XMMatrixTranslation(25, 24, -25 + i * 50), XMMatrixScaling(1, 1, 1), index_cache++);
    }

    const int kNumPyramids = 4;
    const int kHalfNumPyramids = 2;
    //FL pyr
    //front row pyr
    for (int i = 0; i < kNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixRotationY(XMConvertToRadians(-90)), XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(-28 + i * 2, 50, -28), XMMatrixScaling(1, 1, 1), index_cache++); //step = 2, min = -28
    }
    //back row pyr
    for (int i = 0; i < kNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixRotationY(XMConvertToRadians(90)), XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(-28 + i * 2, 50, -22), XMMatrixScaling(1, 1, 1), index_cache++);
    }
    //left col pyr
    for (int i = 0; i < kHalfNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(-28, 50, -26 + i * 2), XMMatrixScaling(1, 1, 1), index_cache++);
    }
    //right col pyr
    for (int i = 0; i < kHalfNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixRotationY(XMConvertToRadians(180)), XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(-22, 50, -26 + i * 2), XMMatrixScaling(1, 1, 1), index_cache++);
    }

    //RL pyr
    //front row pyr
    for (int i = 0; i < kNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixRotationY(XMConvertToRadians(-90)), XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(-28 + i * 2, 50, 28), XMMatrixScaling(1, 1, 1), index_cache++); //step = 2, min = -28
    }
    //back row pyr
    for (int i = 0; i < kNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixRotationY(XMConvertToRadians(90)), XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(-28 + i * 2, 50, 22), XMMatrixScaling(1, 1, 1), index_cache++);
    }
    //left col pyr
    for (int i = 0; i < kHalfNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(-28, 50, 24 + i * 2), XMMatrixScaling(1, 1, 1), index_cache++);
    }
    //right col pyr
    for (int i = 0; i < kHalfNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixRotationY(XMConvertToRadians(180)), XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(-22, 50, 24 + i * 2), XMMatrixScaling(1, 1, 1), index_cache++);
    }

    //FR pyr
    //front row pyr
    for (int i = 0; i < kNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixRotationY(XMConvertToRadians(-90)), XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(22 + i * 2, 50, -28), XMMatrixScaling(1, 1, 1), index_cache++); //step = 2, min = -28
    }
    //back row pyr
    for (int i = 0; i < kNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixRotationY(XMConvertToRadians(90)), XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(22 + i * 2, 50, -22), XMMatrixScaling(1, 1, 1), index_cache++);
    }
    //left col pyr
    for (int i = 0; i < kHalfNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(22, 50, -26 + i * 2), XMMatrixScaling(1, 1, 1), index_cache++);
    }
    //right col pyr
    for (int i = 0; i < kHalfNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixRotationY(XMConvertToRadians(180)), XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(28, 50, -26 + i * 2), XMMatrixScaling(1, 1, 1), index_cache++);
    }

    //RR pyr
    //front row pyr
    for (int i = 0; i < kNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixRotationY(XMConvertToRadians(-90)), XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(22 + i * 2, 50, 22), XMMatrixScaling(1, 1, 1), index_cache++); //step = 2, min = -28
    }
    //back row pyr
    for (int i = 0; i < kNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixRotationY(XMConvertToRadians(90)), XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(22 + i * 2, 50, 28), XMMatrixScaling(1, 1, 1), index_cache++);
    }
    //left col pyr
    for (int i = 0; i < kHalfNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(22, 50, 24 + i * 2), XMMatrixScaling(1, 1, 1), index_cache++);
    }
    //right col pyr
    for (int i = 0; i < kHalfNumPyramids; ++i) {
        BuildOneRenderItem("pyramid", "pyramidGeo", "cutPyr0", XMMatrixRotationY(XMConvertToRadians(180)), XMMatrixScaling(2.4f, 2.4f, 2.4f), XMMatrixTranslation(28, 50, 24 + i * 2), XMMatrixScaling(1, 1, 1), index_cache++);
    }

    const int kNumDiamonds = 2;
    // front charms
    for (int i = 0; i < kNumDiamonds; ++i) {
        BuildOneRenderItem("charm", "charmGeo", "diamond0", XMMatrixRotationRollPitchYaw(XMConvertToRadians(0), XMConvertToRadians(45), XMConvertToRadians(0)), XMMatrixScaling(3, 8, 3), XMMatrixTranslation(-25 + i * 50, 34, -29), XMMatrixScaling(1, 1, 1), index_cache++);
    }
    // back charms
    for (int i = 0; i < kNumDiamonds; ++i) {
        BuildOneRenderItem("charm", "charmGeo", "diamond0", XMMatrixRotationRollPitchYaw(XMConvertToRadians(0), XMConvertToRadians(45), XMConvertToRadians(0)), XMMatrixScaling(3, 8, 3), XMMatrixTranslation(-25 + i * 50, 34, 29), XMMatrixScaling(1, 1, 1), index_cache++);
    }
    // left charms
    for (int i = 0; i < kNumDiamonds; ++i) {
        BuildOneRenderItem("charm", "charmGeo", "diamond0", XMMatrixRotationRollPitchYaw(XMConvertToRadians(0), XMConvertToRadians(45), XMConvertToRadians(0)), XMMatrixScaling(3, 8, 3), XMMatrixTranslation(29, 34, -25 + i * 50), XMMatrixScaling(1, 1, 1), index_cache++);
    }
    // right charms
    for (int i = 0; i < kNumDiamonds; ++i) {
        BuildOneRenderItem("charm", "charmGeo", "diamond0", XMMatrixRotationRollPitchYaw(XMConvertToRadians(0), XMConvertToRadians(45), XMConvertToRadians(0)), XMMatrixScaling(3, 8, 3), XMMatrixTranslation(-29, 34, -25 + i * 50), XMMatrixScaling(1, 1, 1), index_cache++);
    }

   

    //INNER
    // inner building
    BuildOneRenderItem("box", "boxGeo", "inner0", XMMatrixScaling(30, 40, 30), XMMatrixTranslation(0, 20, 0), XMMatrixScaling(1, 1, 1), index_cache++);

    // diamond
    BuildOneRenderItem("diamond", "diamondGeo", "diamond0", XMMatrixScaling(3, 10, 3), XMMatrixTranslation(0.0f, 52, 0.0f), XMMatrixScaling(1, 1, 1), index_cache++);

    // torus
    BuildOneRenderItem("torus", "torusGeo", "brown0", XMMatrixScaling(4, 3, 4), XMMatrixTranslation(0.0f, 40, 0.0f), XMMatrixScaling(1, 1, 1), index_cache++);

    //rolo
    BuildOneRenderItem("rolo", "roloGeo", "red0", XMMatrixScaling(8, 6, 8), XMMatrixTranslation(0, 44, 0), XMMatrixScaling(1, 1, 1), index_cache++); //step = 4, min = -20

    for (int i = 0; i < 2; ++i)
    {
        // left cylinders
        BuildOneRenderItem("cylinder", "cylinderGeo", "cylinder0", XMMatrixScaling(2, 28, 2), XMMatrixTranslation(-15, 27, -15 + i * 30), XMMatrixScaling(1, 1, 1), index_cache++);
        // left cones
        BuildOneRenderItem("cone", "coneGeo", "cone0", XMMatrixScaling(4, 7, 4), XMMatrixTranslation(-15, 60, -15 + i * 30), XMMatrixScaling(1, 1, 1), index_cache++);
        // right cylinders
        BuildOneRenderItem("cylinder", "cylinderGeo", "cylinder0", XMMatrixScaling(2, 28, 2), XMMatrixTranslation(15, 27, -15 + i * 30), XMMatrixScaling(1, 1, 1), index_cache++);
        // right cones
        BuildOneRenderItem("cone", "coneGeo", "cone0", XMMatrixScaling(4, 7, 4), XMMatrixTranslation(15, 60, -15 + i * 30), XMMatrixScaling(1, 1, 1), index_cache++);
    }


    // front prism loop
    for (int i = 0; i < 7; ++i)
    {
        BuildOneRenderItem("prism", "prismGeo", "red0", XMMatrixScaling(2, 2, 2), XMMatrixTranslation(-12 + i * 4, 41, -14), XMMatrixScaling(1, 1, 1), index_cache++); //step = 4, min = -12
    }
    // back prism loop
    for (int i = 0; i < 7; ++i)
    {
        BuildOneRenderItem("prism", "prismGeo", "red0", XMMatrixRotationY(XMConvertToRadians(180.0f)), XMMatrixScaling(2, 2, 2), XMMatrixTranslation(-12 + i * 4, 41, 14), XMMatrixScaling(1, 1, 1), index_cache++); //step = 4, min = -12
    }
    // left prism loop
    for (int i = 0; i < 7; ++i)
    {
        BuildOneRenderItem("prism", "prismGeo", "red0", XMMatrixRotationY(XMConvertToRadians(90.0f)), XMMatrixScaling(2, 2, 2), XMMatrixTranslation(-14, 41, -12 + i * 4), XMMatrixScaling(1, 1, 1), index_cache++);
    }
    // right prism loop
    for (int i = 0; i < 7; ++i)
    {
        BuildOneRenderItem("prism", "prismGeo", "red0", XMMatrixRotationY(XMConvertToRadians(-90.0f)), XMMatrixScaling(2, 2, 2), XMMatrixTranslation(14, 41, -12 + i * 4), XMMatrixScaling(1, 1, 1), index_cache++);
    }


    // MAZE
    
    float scale_offset = 2.0f;
    float move_offset = scale_offset * 0.5f; //spacing for castle walls
    float wall_thickness = scale_offset * 0.3f;
    for (int i = 0; i < kMazeHorizontalSize; i++) {
        BuildOneRenderItem("box", "boxGeo", "inner0", XMMatrixScaling(1 * scale_offset, 1 * scale_offset, wall_thickness ),
            XMMatrixTranslation(kMazeHorizontalCoords[i][0] * scale_offset + move_offset * scale_offset, kMazeHorizontalCoords[i][1], (-kMazeHorizontalCoords[i][2]) * scale_offset),
            XMMatrixScaling(1, 1, wall_thickness), index_cache++);
    }
    for (int i = 0; i < kMazeVerticalSize; i++) {
        BuildOneRenderItem("box", "boxGeo", "inner0", XMMatrixScaling(wall_thickness , 1 * scale_offset, 1 * scale_offset),
            XMMatrixTranslation((kMazeVerticalCoords[i][0]) * scale_offset, kMazeVerticalCoords[i][1], -kMazeVerticalCoords[i][2] * scale_offset + move_offset * scale_offset),
            XMMatrixScaling(wall_thickness, 1, 1), index_cache++);
    }


    // gates
   // BuildOneRenderItem("gate", "gateGeo", "gate0", XMMatrixScaling(16, 24, 5), XMMatrixTranslation(0.0f, 11, -25), XMMatrixScaling(1, 1, 1), index_cache++);
    //BuildOneRenderItem("gate", "gateGeo", "gate0", XMMatrixScaling(10, 20, 5), XMMatrixTranslation(0.0f, 11, -12.7f), XMMatrixScaling(1, 1, 1), index_cache++);

    auto shape_render_item = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&shape_render_item->World, XMMatrixScaling(16, 24, 2) * XMMatrixTranslation(0.0f, 11, -26.1));
    XMStoreFloat4x4(&shape_render_item->TexTransform, XMMatrixScaling(1, 1, 1));
    shape_render_item->ObjCBIndex = index_cache;
    shape_render_item->Mat = mMaterials["gate0"].get();
    shape_render_item->Geo = mGeometries["gateGeo"].get();
    shape_render_item->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    shape_render_item->IndexCount = shape_render_item->Geo->DrawArgs["gate"].IndexCount;
    shape_render_item->StartIndexLocation = shape_render_item->Geo->DrawArgs["gate"].StartIndexLocation;
    shape_render_item->BaseVertexLocation = shape_render_item->Geo->DrawArgs["gate"].BaseVertexLocation;
    index_cache++;

    mRitemLayer[(int)RenderLayer::AlphaTested].push_back(shape_render_item.get());
    mAllRitems.push_back(std::move(shape_render_item));

    /*
            auto shape_render_item = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&shape_render_item->World, scale_matrix * translate_matrix);
    XMStoreFloat4x4(&shape_render_item->TexTransform, tex_scale_matrix);
    shape_render_item->ObjCBIndex = obj_idx;
    shape_render_item->Mat = mMaterials[material].get();
    shape_render_item->Geo = mGeometries[shape_name].get();
    shape_render_item->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    shape_render_item->IndexCount = shape_render_item->Geo->DrawArgs[shape_type].IndexCount;
    shape_render_item->StartIndexLocation = shape_render_item->Geo->DrawArgs[shape_type].StartIndexLocation;
    shape_render_item->BaseVertexLocation = shape_render_item->Geo->DrawArgs[shape_type].BaseVertexLocation;
    mRitemLayer[(int)RenderLayer::Opaque].push_back(shape_render_item.get());
    mAllRitems.push_back(std::move(shape_render_item));
    
    */

    auto treeSpritesRitem = std::make_unique<RenderItem>();
    treeSpritesRitem->World = MathHelper::Identity4x4();
    treeSpritesRitem->ObjCBIndex = index_cache;
    treeSpritesRitem->Mat = mMaterials["treeSprites"].get();
    treeSpritesRitem->Geo = mGeometries["treeSpritesGeo"].get();
    //step2
    treeSpritesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs["points"].IndexCount;
    treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
    treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;
    index_cache++;

    mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(treeSpritesRitem.get());
    mAllRitems.push_back(std::move(treeSpritesRitem));

    // CLOUDS
    auto cloudSpritesRitem = std::make_unique<RenderItem>();
    cloudSpritesRitem->World = MathHelper::Identity4x4();
    cloudSpritesRitem->ObjCBIndex = index_cache;
    cloudSpritesRitem->Mat = mMaterials["cloudSprites"].get();
    cloudSpritesRitem->Geo = mGeometries["cloudSpritesGeo"].get();
    cloudSpritesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    cloudSpritesRitem->IndexCount = cloudSpritesRitem->Geo->DrawArgs["points"].IndexCount;
    cloudSpritesRitem->StartIndexLocation = cloudSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
    cloudSpritesRitem->BaseVertexLocation = cloudSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;
    index_cache++;

    mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(cloudSpritesRitem.get());
    mAllRitems.push_back(std::move(cloudSpritesRitem));
    
    // WYVERN
    auto wyvernSpritesRitem = std::make_unique<RenderItem>();
    wyvernSpritesRitem->World = MathHelper::Identity4x4();
    wyvernSpritesRitem->ObjCBIndex = index_cache;
    wyvernSpritesRitem->Mat = mMaterials["wyvernSprites"].get();
    wyvernSpritesRitem->Geo = mGeometries["wyvernSpritesGeo"].get();
    wyvernSpritesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    wyvernSpritesRitem->IndexCount = wyvernSpritesRitem->Geo->DrawArgs["points"].IndexCount;
    wyvernSpritesRitem->StartIndexLocation = wyvernSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
    wyvernSpritesRitem->BaseVertexLocation = wyvernSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;
    index_cache++;

    mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(wyvernSpritesRitem.get());
    mAllRitems.push_back(std::move(wyvernSpritesRitem));

    //// All the render items are opaque.
    //for(auto& e : mAllRitems)
    //	mOpaqueRitems.push_back(e.get());

    ::OutputDebugStringA(">>> BuildRenderItems DONE!\n");
}


//The DrawRenderItems method is invoked in the main Draw call:
void ShapesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

    auto objectCB = mCurrFrameResource->ObjectCB->Resource();
    auto matCB = mCurrFrameResource->MaterialCB->Resource();

    // For each render item...
    for (size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
        D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

        cmdList->SetGraphicsRootDescriptorTable(0, tex);
        cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> ShapesApp::GetStaticSamplers()
{
    // Applications usually only need a handful of samplers.  So just define them all up front
    // and keep them available as part of the root signature.  

    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
        0, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
        1, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
        2, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        3, // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
        4, // shaderRegister
        D3D12_FILTER_ANISOTROPIC, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
        0.0f,                             // mipLODBias
        8);                               // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
        5, // shaderRegister
        D3D12_FILTER_ANISOTROPIC, // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
        0.0f,                              // mipLODBias
        8);                                // maxAnisotropy

    return {
        pointWrap, pointClamp,
        linearWrap, linearClamp,
        anisotropicWrap, anisotropicClamp };
}

float ShapesApp::GetHillsHeight(float x, float z)const
{
    return 0.15f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 ShapesApp::GetHillsNormal(float x, float z)const
{
    // n = (-df/dx, 1, -df/dz)
    XMFLOAT3 n(
        -0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
        1.0f,
        -0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

    XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
    XMStoreFloat3(&n, unitNormal);

    return n;
}
