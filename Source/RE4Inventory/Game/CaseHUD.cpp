#include "Game/CaseHUD.h"
#include "Game/CaseInventoryController.h"
#include "View/CaseViewActor.h"
#include "Inventory/CaseCursor.h"
#include "Inventory/CaseGrid.h"
#include "Inventory/ItemDefinition.h"
#include "Engine/Engine.h"

void ACaseHUD::DrawHUD()
{
	Super::DrawHUD();

	const ACaseInventoryController* Controller = Cast<ACaseInventoryController>(GetOwningPlayerController());
	const ACaseViewActor* Case = Controller ? Controller->GetCase() : nullptr;
	UFont* Font = GEngine->GetMediumFont();
	if (!Case || !Case->GetGrid() || !Case->GetCursor())
	{
		DrawText(TEXT("No ACaseViewActor in this level"), FLinearColor::Red, 20.0f, 20.0f, Font);
		return;
	}

	const UCaseGrid* Grid = Case->GetGrid();
	const UCaseCursor* Cursor = Case->GetCursor();
	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("ATTACHE CASE  %d x %d"), Grid->GetWidth(), Grid->GetHeight()));
	Lines.Add(TEXT("LMB pick/place   RMB or R rotate   C cancel   X discard"));
	Lines.Add(TEXT("Z undo   1 arrange (space)   2 arrange (category)   E expand"));

	FInventoryItem Item;
	if (Cursor->IsHolding() && Grid->GetItem(Cursor->GetHeldId(), Item) && Item.Definition)
	{
		Lines.Add(FString::Printf(TEXT("Holding: %s x%d  ->  %s"), *Item.Definition->DisplayName.ToString(), Item.Count,
			Cursor->CanDropHere() ? TEXT("OK") : TEXT("BLOCKED")));
	}
	else if (Grid->GetItem(Grid->GetItemAt(Cursor->GetHoverCell()), Item) && Item.Definition)
	{
		Lines.Add(FString::Printf(TEXT("%s x%d  (%d x %d)"), *Item.Definition->DisplayName.ToString(), Item.Count,
			Item.Definition->Size.X, Item.Definition->Size.Y));
	}
	if (!Case->GetStatusText().IsEmpty())
	{
		Lines.Add(Case->GetStatusText());
	}
	Lines.Add(FString::Printf(TEXT("Undo available: %s"), Grid->CanUndo() ? TEXT("yes") : TEXT("no")));

	float Y = 20.0f;
	for (const FString& Line : Lines)
	{
		DrawText(Line, FLinearColor(0.92f, 0.88f, 0.75f), 20.0f, Y, Font);
		Y += 22.0f;
	}
}
