#include "pch.h"
#include "AnimatorComponent.h"
#include "Time.h"
#include "Actor.h"
#include "IModel.h"
#include "AssimpModel.h"

// 指定したノード名のアニメーションデータを探す
const aiNodeAnim* AnimatorComponent::FindNodeAnim(const aiAnimation* pAnimation, const std::string& nodeName)
{
	if (!pAnimation) return nullptr;

	unsigned int channel_count = pAnimation->mNumChannels;
	aiNodeAnim** channels = pAnimation->mChannels;

	for (unsigned int i = 0; i < channel_count; i++) {
		// std::string の == 演算子を利用して中身の文字列比較を行う
		if (nodeName == channels[i]->mNodeName.C_Str()) {
			return channels[i];
		}
	}

	return nullptr;
}

DirectX::XMVECTOR AnimatorComponent::CalcInterpolatedPosition(float animationTime, const aiNodeAnim* pNodeAnim) {
	if (pNodeAnim->mNumPositionKeys == 0) return DirectX::XMVectorZero();
	if (pNodeAnim->mNumPositionKeys == 1) {
		auto pos = pNodeAnim->mPositionKeys[0].mValue;
		return DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
	}

	unsigned int index = FindPositionKey(animationTime, pNodeAnim);
	unsigned int nextIndex = index + 1;
	auto startKey = pNodeAnim->mPositionKeys[index];
	auto endKey = pNodeAnim->mPositionKeys[nextIndex];

	float deltaTime = static_cast<float>(endKey.mTime - startKey.mTime);
	float factor = 0.0f;
	if (deltaTime > 0.0f) {
		factor = (animationTime - static_cast<float>(startKey.mTime)) / deltaTime;
		if (factor < 0.0f) factor = 0.0f;
		if (factor > 1.0f) factor = 1.0f;
	}

	DirectX::XMVECTOR startVec = DirectX::XMVectorSet(startKey.mValue.x, startKey.mValue.y, startKey.mValue.z, 1.0f);
	DirectX::XMVECTOR endVec = DirectX::XMVectorSet(endKey.mValue.x, endKey.mValue.y, endKey.mValue.z, 1.0f);

	return DirectX::XMVectorLerp(startVec, endVec, factor);
}

DirectX::XMVECTOR AnimatorComponent::CalcInterpolatedRotation(float animationTime, const aiNodeAnim* pNodeAnim) {
	if (pNodeAnim->mNumRotationKeys == 0) return DirectX::XMQuaternionIdentity();
	if (pNodeAnim->mNumRotationKeys == 1) {
		auto rot = pNodeAnim->mRotationKeys[0].mValue;
		return DirectX::XMVectorSet(rot.x, rot.y, rot.z, rot.w);
	}

	unsigned int index = FindRotationKey(animationTime, pNodeAnim);
	unsigned int nextIndex = index + 1;
	auto startKey = pNodeAnim->mRotationKeys[index];
	auto endKey = pNodeAnim->mRotationKeys[nextIndex];

	float deltaTime = static_cast<float>(endKey.mTime - startKey.mTime);
	float factor = 0.0f;
	if (deltaTime > 0.0f) {
		factor = (animationTime - static_cast<float>(startKey.mTime)) / deltaTime;
		if (factor < 0.0f) factor = 0.0f;
		if (factor > 1.0f) factor = 1.0f;
	}

	DirectX::XMVECTOR startVec = DirectX::XMVectorSet(startKey.mValue.x, startKey.mValue.y, startKey.mValue.z, startKey.mValue.w);
	DirectX::XMVECTOR endVec = DirectX::XMVectorSet(endKey.mValue.x, endKey.mValue.y, endKey.mValue.z, endKey.mValue.w);

	return DirectX::XMQuaternionSlerp(startVec, endVec, factor);
}

DirectX::XMVECTOR AnimatorComponent::CalcInterpolatedScaling(float animationTime, const aiNodeAnim* pNodeAnim) {
	if (pNodeAnim->mNumScalingKeys == 0) return DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	if (pNodeAnim->mNumScalingKeys == 1) {
		auto pos = pNodeAnim->mScalingKeys[0].mValue;
		return DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
	}

	unsigned int index = FindScalingKey(animationTime, pNodeAnim);
	unsigned int nextIndex = index + 1;
	auto startKey = pNodeAnim->mScalingKeys[index];
	auto endKey = pNodeAnim->mScalingKeys[nextIndex];

	float deltaTime = static_cast<float>(endKey.mTime - startKey.mTime);
	float factor = 0.0f;
	if (deltaTime > 0.0f) {
		factor = (animationTime - static_cast<float>(startKey.mTime)) / deltaTime;
		if (factor < 0.0f) factor = 0.0f;
		if (factor > 1.0f) factor = 1.0f;
	}

	DirectX::XMVECTOR startVec = DirectX::XMVectorSet(startKey.mValue.x, startKey.mValue.y, startKey.mValue.z, 1.0f);
	DirectX::XMVECTOR endVec = DirectX::XMVectorSet(endKey.mValue.x, endKey.mValue.y, endKey.mValue.z, 1.0f);

	return DirectX::XMVectorLerp(startVec, endVec, factor);
}

// 指定した時間に対応する位置キーフレームのインデックスを探す
unsigned int AnimatorComponent::FindPositionKey(float animationTime, const aiNodeAnim* pNodeAnim) {
	unsigned int key_count = pNodeAnim->mNumPositionKeys;
	for (unsigned int i = 0; i < key_count - 1; i++) {
		if (pNodeAnim->mPositionKeys[i + 1].mTime >= static_cast<double>(animationTime)) {
			return i;
		}
	}
	return (key_count > 1) ? (key_count - 2) : 0;
}

// 指定した時間に対応する回転キーフレームのインデックスを探す
unsigned int AnimatorComponent::FindRotationKey(float animationTime, const aiNodeAnim* pNodeAnim) {
	unsigned int key_count = pNodeAnim->mNumRotationKeys;
	for (unsigned int i = 0; i < key_count - 1; i++) {
		if (pNodeAnim->mRotationKeys[i + 1].mTime >= static_cast<double>(animationTime)) {
			return i;
		}
	}
	return (key_count > 1) ? (key_count - 2) : 0;
}

// 指定した時間に対応するスケールキーフレームのインデックスを探す
unsigned int AnimatorComponent::FindScalingKey(float animationTime, const aiNodeAnim* pNodeAnim) {
	unsigned int key_count = pNodeAnim->mNumScalingKeys;
	for (unsigned int i = 0; i < key_count - 1; i++) {
		if (pNodeAnim->mScalingKeys[i + 1].mTime >= static_cast<double>(animationTime)) {
			return i;
		}
	}
	return (key_count > 1) ? (key_count - 2) : 0;
}

bool AnimatorComponent::CheckAssimpModel(IModel* model, AssimpModel** result)
{
	if (result == nullptr) return false;
	*result = dynamic_cast<AssimpModel*>(model);
	if (*result != nullptr) {
		return true;
	}

	return false;
}

void AnimatorComponent::UpdateBoneTransforms(AssimpModel* model, float animationTime, const aiAnimation* pAnim, const aiNode* pNode, const DirectX::XMMATRIX& parentTransform)
{
	std::string nodeName = pNode->mName.C_Str();
	DirectX::XMMATRIX localTransform;

	const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnim, nodeName);

	if (pNodeAnim) {
		// アニメーションデータがある場合: キーフレームから補間して変換行列を組み立てる

		// 初期ポーズの行列からスケール・回転・位置を分解
		aiVector3D defaultScale, defaultPos;
		aiQuaternion defaultRot;
		pNode->mTransformation.Decompose(defaultScale, defaultRot, defaultPos);

		// スケール: キーフレームが2つ以上あれば補間値を使用、それ以外は初期ポーズを使用
		DirectX::XMVECTOR scaleVec = (pNodeAnim->mNumScalingKeys > 1) ? CalcInterpolatedScaling(animationTime, pNodeAnim) : DirectX::XMVectorSet(defaultScale.x, defaultScale.y, defaultScale.z, 1.0f);
		DirectX::XMFLOAT3 s;
		DirectX::XMStoreFloat3(&s, scaleVec);
		DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(s.x, s.y, s.z);

		// 回転: キーフレームが2つ以上あれば補間値を使用、それ以外は初期ポーズを使用
		DirectX::XMVECTOR rotQuat = (pNodeAnim->mNumRotationKeys > 1) ? CalcInterpolatedRotation(animationTime, pNodeAnim) : DirectX::XMVectorSet(defaultRot.x, defaultRot.y, defaultRot.z, defaultRot.w);
		DirectX::XMMATRIX rotMatrix = DirectX::XMMatrixRotationQuaternion(rotQuat);

		// 位置: キーフレームが2つ以上あれば補間値を使用、それ以外は初期ポーズを使用
		DirectX::XMVECTOR posVec = (pNodeAnim->mNumPositionKeys > 1) ? CalcInterpolatedPosition(animationTime, pNodeAnim) : DirectX::XMVectorSet(defaultPos.x, defaultPos.y, defaultPos.z, 1.0f);
		DirectX::XMFLOAT3 p;
		DirectX::XMStoreFloat3(&p, posVec);
		DirectX::XMMATRIX transMatrix = DirectX::XMMatrixTranslation(p.x, p.y, p.z);

		// SRT行列を合成（Scale → Rotation → Translation の順）
		localTransform = DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(scaleMatrix, rotMatrix), transMatrix);
	}
	else {
		// アニメーションデータがない場合: ノードの初期変換行列をそのまま使用

		aiMatrix4x4 aiLocalTransform = pNode->mTransformation;
		// Assimpの行列をXMMATRIXにコピー
		localTransform = DirectX::XMMATRIX(
			aiLocalTransform.a1, aiLocalTransform.a2, aiLocalTransform.a3, aiLocalTransform.a4,
			aiLocalTransform.b1, aiLocalTransform.b2, aiLocalTransform.b3, aiLocalTransform.b4,
			aiLocalTransform.c1, aiLocalTransform.c2, aiLocalTransform.c3, aiLocalTransform.c4,
			aiLocalTransform.d1, aiLocalTransform.d2, aiLocalTransform.d3, aiLocalTransform.d4
		);

		// Assimp（列ベクトル規約）→ DirectX（行ベクトル規約）への変換のため転置
		localTransform = DirectX::XMMatrixTranspose(localTransform);
	}

	DirectX::XMMATRIX globalTransform = DirectX::XMMatrixMultiply(localTransform, parentTransform);

	const auto& boneMap = model->GetBoneMap();

	auto it = boneMap.find(nodeName);
	if (it != boneMap.end()) {
		int boneIndex = it->second.id;
		aiMatrix4x4 offset = it->second.offsetMatrix;

		DirectX::XMMATRIX offsetMatrix = DirectX::XMMATRIX(
			offset.a1, offset.a2, offset.a3, offset.a4,
			offset.b1, offset.b2, offset.b3, offset.b4,
			offset.c1, offset.c2, offset.c3, offset.c4,
			offset.d1, offset.d2, offset.d3, offset.d4);

		// OffsetMatrixもAssimp由来のため転置が必要
		offsetMatrix = DirectX::XMMatrixTranspose(offsetMatrix);

		// 最終行列 = OffsetMatrix * GlobalTransform
		DirectX::XMMATRIX finalTransform = DirectX::XMMatrixMultiply(offsetMatrix, globalTransform);

		// ここでも転置が必要になる？
		// finalTransformは既に転置されたものを使っているのに？
		_owner->SetBoneTransform(boneIndex, DirectX::XMMatrixTranspose(finalTransform));
	}

	for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
		UpdateBoneTransforms(model, animationTime, pAnim, pNode->mChildren[i], globalTransform);
	}
}

AnimatorComponent::AnimatorComponent(Actor* owner)
	: BaseComponent(owner)
	, _animationTime(0.0f)
{
}

AnimatorComponent::~AnimatorComponent()
{
}

void AnimatorComponent::Update()
{
	IModel* model = _owner->GetModel();
	if (!model) return;

	AssimpModel* assimpModel = nullptr;
	if (!CheckAssimpModel(model, &assimpModel)) {
		return;
	}

	const aiScene* scene = assimpModel->GetScene();
	if (!scene || !scene->HasAnimations()) return;

	const aiAnimation* pAnim = scene->mAnimations[0];

	float deltaTimeInSeconds = Time::GetDeltaTime();
	float ticksPerSecond = static_cast<float>(pAnim->mTicksPerSecond != 0 ? pAnim->mTicksPerSecond : 25.0f);
	float timeInTicks = deltaTimeInSeconds * ticksPerSecond;

	_animationTime += timeInTicks;

	if (pAnim->mDuration > 0.0) {
		_animationTime = fmod(_animationTime, static_cast<float>(pAnim->mDuration));
	}
	else {
		_animationTime = 0.0f;
	}

	UpdateBoneTransforms(assimpModel, _animationTime, pAnim, scene->mRootNode, DirectX::XMMatrixIdentity());
}