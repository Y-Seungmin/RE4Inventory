#pragma once

#include "CoreMinimal.h"

/** 아이템 모양 계산을 한곳에 모은다. 배치 검사와 화면 표시가 같은 식을 쓰게 하기 위함 */
namespace ItemShape
{
	/** 회전 단계(0~3, 90도씩)를 반영한 가로 x 세로. 홀수 단계면 가로세로가 바뀐다 */
	inline FIntPoint GetFootprint(FIntPoint BaseSize, int32 Rotation)
	{
		return (Rotation & 1) ? FIntPoint(BaseSize.Y, BaseSize.X) : BaseSize;
	}

	/** 0~3 범위로 맞춘 회전 단계 */
	inline int32 NormalizeRotation(int32 Rotation)
	{
		return ((Rotation % 4) + 4) % 4;
	}

	/**
	 * 시계 방향 90도 회전. 크기 W x H 안의 칸 (x, y)는 크기 H x W 안의 (H-1-y, x)로 간다.
	 * 드래그 중 회전할 때, 마우스로 잡은 칸의 위치를 같이 돌리는 데 쓴다.
	 */
	inline FIntPoint Rotate90(FIntPoint Cell, FIntPoint SizeBefore)
	{
		return FIntPoint(SizeBefore.Y - 1 - Cell.Y, Cell.X);
	}
}
