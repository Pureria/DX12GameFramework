#pragma once
#include <string>
#include <assimp/scene.h>
#include <DirectXMath.h>
#include "BaseComponent.h"

class IModel;
class AssimpModel;

class AnimatorComponent : public BaseComponent
{
private:
	float _animationTime;

	void UpdateBoneTransforms(AssimpModel* model, float animationTime, const aiAnimation* pAnim, const aiNode* pNode, const DirectX::XMMATRIX& parentTransform);

	// 指定したノード名のアニメーションデータを探す
	const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string& nodeName);

	// 指定した時間の補間された位置・回転・スケールを計算する
	DirectX::XMVECTOR CalcInterpolatedPosition(float animationTime, const aiNodeAnim* pNodeAnim);
	DirectX::XMVECTOR CalcInterpolatedRotation(float animationTime, const aiNodeAnim* pNodeAnim);
	DirectX::XMVECTOR CalcInterpolatedScaling(float animationTime, const aiNodeAnim* pNodeAnim);

	// 指定した時間に対応する Position/Rotation/Scaling キーフレームのインデックスを探す
	unsigned int FindPositionKey(float animationTime, const aiNodeAnim* pNodeAnim);
	unsigned int FindRotationKey(float animationTime, const aiNodeAnim* pNodeAnim);
	unsigned int FindScalingKey(float animationTime, const aiNodeAnim* pNodeAnim);

	bool CheckAssimpModel(IModel* model, AssimpModel** result);
public:
	AnimatorComponent(Actor* owner);
	~AnimatorComponent() override;

	void Update() override;
};

