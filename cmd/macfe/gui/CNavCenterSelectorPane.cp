/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "CNavCenterSelectorPane.h"

#include <algorithm>
#include <LStream.h>

#include "CRDFCoordinator.h"
#include "CContextMenuAttachment.h"
#include "Netscape_constants.h"


#pragma mark -- CNavCenterSelectorPane methods --


const RGBColor CNavCenterSelectorPane::mBGColor = { 0xDDDD, 0xDDDD, 0xDDDD };


CNavCenterSelectorPane::CNavCenterSelectorPane( LStream* inStream )
		: LView(inStream), LDragAndDrop ( GetMacPort(), this ),
			mIsEmbedded(false), mHTPane(nil), mTooltipIndex(-1), mMouseIsInsidePane(false),
			mImageMode(0L), mCellHeight(25), mCachedWorkspace(NULL)
{
	// nothing else to do here.
}


CNavCenterSelectorPane::~CNavCenterSelectorPane()
{
	// nothing else to do here since all FE data is cleaned up by workspace remove events
}


//
// SetActiveWorkspace
//
// Call this to change the current workspace. Passing in NULL will clear the selection. Most
// of the work is done in NoticeActiveWorkspaceChanged() where the view is redrawn and the
// view change is broadcast to the RDFCoordinator. This doesn't actually change HT's
// currently selected view, as it is done by the coordinator.
//
void
CNavCenterSelectorPane :: SetActiveWorkspace ( HT_View inNewWorkspace )
{
	HT_View oldWorkspace = mCachedWorkspace;
	mCachedWorkspace = inNewWorkspace;
	NoticeActiveWorkspaceChanged(oldWorkspace);

} // SetActiveWorkspace


//
// DrawSelf
//
// Iterate over each view and draw it. We store the class that is responsible for drawing a 
// workspace in the FE data in HT.
//
void
CNavCenterSelectorPane::DrawSelf()
{
	Rect cellBounds;
	CalcLocalFrameRect(cellBounds);
	
	// erase the background
	StColorState saved;
	::RGBBackColor(&mBGColor);
	::EraseRect(&cellBounds);
	
	// find the bounds of the first cell
	cellBounds.bottom = cellBounds.top + mCellHeight;

#if DRAW_WITH_TITLE
	StTextState savedState;
	UTextTraits::SetPortTextTraits(130);
#endif
		
	// iterate over workspaces, drawing each in turn.
	const listCount = HT_GetViewListCount(GetHTPane());
	const HT_View selectedView = HT_GetSelectedView(GetHTPane());
	for ( int i = 0; i < listCount; i++ ) {
		unsigned long drawMode = mImageMode;

		HT_View currView = HT_GetNthView(GetHTPane(), i);
		SelectorData* viewData = static_cast<SelectorData*>(HT_GetViewFEData(currView));
		if ( viewData ) {
			if ( currView == selectedView )
				drawMode |= kSelected;
			viewData->workspaceImage->DrawInCurrentView(cellBounds, drawMode);
		}
		
		// prepare to draw next selector
		cellBounds.top += mCellHeight;
		cellBounds.bottom += mCellHeight;

	} // for each selector
	
} // DrawSelf


//
// ClickSelf
//
// Handle when the user clicks in this pane. A click on a workspace icon will switch to that
// view. Also handle opening and closing the shelf (if we're embedded) and context clicking.
//
void
CNavCenterSelectorPane::ClickSelf( const SMouseDownEvent& inMouseDown )
{
	FocusDraw();	// to make the context menu stuff works

	CContextMenuAttachment::SExecuteParams params;
	params.inMouseDown = &inMouseDown;
				
	// do nothing if click not in any workspace except show context menu.
	HT_View clickedView = FindSelectorForPoint(inMouseDown.whereLocal);
	if ( !clickedView ) {
		ExecuteAttachments(CContextMenuAttachment::msg_ContextMenu, (void*)&params);
		return;
	}
	
	// if the user clicks on a workspace icon, open the shelf to display the workspace. If
	// they click on the same workspace icon as is already displayed, close the shelf. Don't
	// change the shelf state on a context click.
	if ( clickedView != GetActiveWorkspace() ) {
		HT_View oldSelection = GetActiveWorkspace();
		SetActiveWorkspace(clickedView);

		ExecuteAttachments(CContextMenuAttachment::msg_ContextMenu, (void*)&params);
		if ( params.outResult != CContextMenuAttachment::eHandledByAttachment && mIsEmbedded ) {
			bool value = true;
			BroadcastMessage ( msg_ShelfStateShouldChange, &value );		// force open
		}
		else {
			// this combats the problem when there is no workspace and the user context clicks. We
			// select the one they clicked on, but don't open the shelf. We can't just leave the
			// item selected and the shelf closed, so we just reset the active workspace to none.
			// Icky code but it doesn't freak out the user. 
			if ( !oldSelection )
				SetActiveWorkspace(NULL);
		}
	}
	else {
		ExecuteAttachments(CContextMenuAttachment::msg_ContextMenu, (void*)&params);
		if ( params.outResult != CContextMenuAttachment::eHandledByAttachment && mIsEmbedded ) {
			bool value = false;
			BroadcastMessage ( msg_ShelfStateShouldChange, &value );		// force closed
			SetActiveWorkspace( NULL );
		}
	}
	
} // ClickSelf


//
// AdjustCursorSelf
//
// Use the context menu attachment to handle showing context menu cursor when cmd key is
// down and mouse is over this view.
//
void
CNavCenterSelectorPane::AdjustCursorSelf( Point /*inPoint*/, const EventRecord& inEvent )
{
	ExecuteAttachments(CContextMenuAttachment::msg_ContextMenuCursor, 
								static_cast<void*>(const_cast<EventRecord*>(&inEvent)));

} // AdjustCursorSelf


//
// FindTooltipForMouseLocation
//
// Provide the tooltip for the workspace that the mouse is hovering over
//
void
CNavCenterSelectorPane :: FindTooltipForMouseLocation ( const EventRecord& inMacEvent,
														StringPtr outTip )
{
	Point temp = inMacEvent.where;
	GlobalToPortPoint(temp);
	PortToLocalPoint(temp);
	
	HT_View selector = FindSelectorForPoint ( temp );
	if ( selector ) {
		// pull the name out of HT and stuff it into the Pascal string...DO NOT DELETE |buffer|.
		const char* buffer = HT_GetViewName ( selector );
		outTip[0] = strlen(buffer);
		strcpy ( (char*) &outTip[1], buffer );
	} 
	else
		::GetIndString ( outTip, 10506, 15);				// supply a helpful message...

} // FindTooltipForMouseLocation


//
// MouseLeave
//
// Called when the mouse leaves the view. Just update our "hot" cell to an invalid cell.
//
void 
CNavCenterSelectorPane :: MouseLeave ( ) 
{
	mTooltipIndex = -1;
	mMouseIsInsidePane = false;
		
} // MouseLeave


//
// MouseWithin
//
// Called while the mouse moves w/in the pane. Find which workspace the mouse is
// currently over and if it differs from the last workspace it was in, hide the 
// tooltip and remember the new cell. 
//
void 
CNavCenterSelectorPane :: MouseWithin ( Point inPortPt, const EventRecord& /*inEvent*/ ) 
{
	mMouseIsInsidePane = true;
	
	size_t index = FindIndexForPoint ( inPortPt );
	if ( mTooltipIndex != index ) {
		mTooltipIndex = index;		
		ExecuteAttachments(msg_HideTooltip, this);	// hide tooltip
	}
	
} // MouseWithin


//
// NoticeActiveWorkspaceChanged
//
// Redraw the old and new workspace icons only when there is a change. Also tell the RDFCoordinator
// to update HT's active workspace.
//
void
CNavCenterSelectorPane :: NoticeActiveWorkspaceChanged ( HT_View inOldSel )
{
	BroadcastMessage(msg_ActiveSelectorChanged, GetActiveWorkspace());

	FocusDraw();
	
#if DRAW_WITH_TITLE
	StTextState savedState;
	::TextFont(kFontIDGeneva);
	::TextSize(9);
	::TextFace(0);
#endif
	
	// redraw old
	Rect previous = { 0, 0, 0, 0 };
	if ( inOldSel ) {
		CalcLocalFrameRect(previous);
		previous.top = HT_GetViewIndex(inOldSel) * mCellHeight;
		previous.bottom = previous.top + mCellHeight;
		SelectorData* data = static_cast<SelectorData*>(HT_GetViewFEData(inOldSel));
		data->workspaceImage->DrawInCurrentView ( previous, mImageMode );
	}
	
	// redraw new
	Rect current = { 0, 0, 0, 0 };
	if ( GetActiveWorkspace() ) {
		CalcLocalFrameRect(current);
		current.top = HT_GetViewIndex(GetActiveWorkspace()) * mCellHeight;
		current.bottom = current.top + mCellHeight;
		SelectorData* data = static_cast<SelectorData*>(HT_GetViewFEData(GetActiveWorkspace()));
		data->workspaceImage->DrawInCurrentView ( current, mImageMode | kSelected );
	}
	
	
} // NoticeActiveWorkspaceChanged


//
// ItemIsAcceptable
//
// Just about anything should be acceptable because we just want to make sure
// that the appropriate nav center view is open for the user to then drag into. Drops
// can occur here, so I guess we have to make sure that it is something that we
// recognize (TEXT, for example, should not be allowed).
//
Boolean
CNavCenterSelectorPane :: ItemIsAcceptable ( DragReference /*inDragRef*/, ItemReference /*inItemRef*/ )
{
	return true;	// for now....
		
} // ItemIsAcceptable


//
// EnterDropArea
//
// If the drag comes from an outside source, record when the mouse entered this pane
//
void
CNavCenterSelectorPane :: EnterDropArea ( DragReference /* inDragRef */, Boolean inDragHasLeftSender )
{
	// if the drag did not originate here, go into "timed switching" mode
	if ( inDragHasLeftSender )
		mDragAndDropTickCount = ::TickCount();

} // EnterDropArea


//
// LeaveDropArea
//
// Reset our counter
//
void
CNavCenterSelectorPane :: LeaveDropArea ( DragReference /* inDragRef */ )
{
	mDragAndDropTickCount = 0L;

} // LeaveDropArea


//
// InsideDropArea
//
// Called when the mouse moves inside this pane on a drag and drop.
//
// There are two cases here: the drag originated here or it didn't. When it did, the user
// is trying to rearrange the order of the views. When the drag comes from outside, we need
// to do our fancy timed switching behavior. We can tell the difference between these two
// modes by checking the value of |mDragAndDropTickCount| which will be non-zero if the 
// drag originated outside.
//
void
CNavCenterSelectorPane :: InsideDropArea ( DragReference inDragRef )
{
	const Uint32 kPaneSwitchDelay = 60;		// 1 second delay
	
	if ( mDragAndDropTickCount ) {
	
		FocusDraw();
		Point mouseLoc;
		::GetDragMouse(inDragRef, &mouseLoc, NULL);
		::GlobalToLocal(&mouseLoc);

		// if the user has dragged down into a place where there are no
		// workspaces, don't do anything.
		HT_View newSelection = FindSelectorForPoint(mouseLoc);
		if ( !newSelection ) {
			mDragAndDropTickCount = ::TickCount();		// see comment below
			return;
		}
			
		// if the user hasn't sat here long enough, don't do anything...
		if ( ::TickCount() - mDragAndDropTickCount < kPaneSwitchDelay )
			return;
		
		// ...ok, they really want to switch panes...
				
		// make sure the shelf is visible (only necessary if in chrome). This
		// shouldn't be necessary, but things won't redraw correctly unless we do this.
		if ( mIsEmbedded ) {
			bool value = true;
			BroadcastMessage ( msg_ShelfStateShouldChange, &value );		// force open
		}
		
		// switch to the workspace the user is now hovering over
		if ( newSelection != GetActiveWorkspace() )
			SetActiveWorkspace ( newSelection );

		// we need to reset the timer so we don't get into the situation where the user
		// hovers over the selected item for a long time and then when they move to the next
		// selector, the time will be > |kPaneSwitchDelay| and it will instantly switch.
		// Keeping our timer up to date should avoid this and force the user to always
		// wait the delay. NOTE: we don't get here if they have yet to reach the delay amount
		// so we're not actually reseting the timer every time this is called.
		mDragAndDropTickCount = ::TickCount();

	} // if we are tracking an outside drag.
	else {
		// do nothing as of yet...
	}
	
} // InsideDropArea


//
// CycleCurrentWorkspace
//
// If the shelf is open, advance the current workspace, wrapping around if we get to the end
//
void
CNavCenterSelectorPane :: CycleCurrentWorkspace ( )
{
	if ( GetActiveWorkspace() ) {
		HT_View newSelector = HT_GetNthView ( GetHTPane(), 
									HT_GetViewIndex(GetActiveWorkspace()) + 1 );
		if ( !newSelector )
			newSelector = HT_GetNthView ( GetHTPane(), 0 );
	
		SetActiveWorkspace(newSelector);
	} // if there is a selection

} // CycleCurrentWorkspace


//
// FindIndexForPoint
//
// Given a point in image coordinates, find the index of the selector the mouse was over. Right
// now, this is just an easy division because the selector regions all butt up against each
// other. If there were ever any space between them, this would need to be more complicated.
//
// This index is 0-based, not 1-based like most other things in Powerplant. This is ok because
// HT is also 0-based and we only use this index when talking to HT. 
//
size_t
CNavCenterSelectorPane :: FindIndexForPoint ( const Point & inMouseLoc ) const
{
	return inMouseLoc.v / mCellHeight;
	
} // FindIndexForPoint


//
// FindSelectorForPoint
//
// Grabs the view where the user clicked from HT and returns it.
//
HT_View
CNavCenterSelectorPane :: FindSelectorForPoint ( const Point & inMouseLoc ) const
{
	return HT_GetNthView ( GetHTPane(), FindIndexForPoint(inMouseLoc) );
	
} // FindSelectorForPoint


//
// FindCommandStatus
//
void
CNavCenterSelectorPane :: FindCommandStatus ( CommandT inCommand, Boolean &outEnabled,
										Boolean &outUsesMark, Char16 &outMark, Str255 outName)
{
	if ( inCommand >= cmd_NavCenterBase && inCommand <= cmd_NavCenterCap )
		outEnabled = HT_IsMenuCmdEnabled(GetHTPane(), (HT_MenuCmd)(inCommand - cmd_NavCenterBase));
	else
		LCommander::FindCommandStatus(inCommand, outEnabled, outUsesMark, outMark, outName);
	
} // FindCommandStatus


#pragma mark --- struct SelectorData ---


SelectorData :: SelectorData ( HT_View inView )
{
	const ResIDT cFolderIconID = -3999;		// use the OS Finder folder icon
	enum { kWorkspaceID = cFolderIconID, kHistoryID = 16251, kSearchID = 16252, kSiteMapID = 16253 };

	char viewName[64];
#if DRAW_WITH_TITLE
	HT_ViewDisplayString ( inView, viewName, 8 );
	viewName[8] = NULL;
#else
	viewName[0] = NULL;
#endif
	
	// ₯₯₯ HACK: figure out what pane this is and give it an icon. We should just be
	// using the url given and pass that to the imageLib, but the API is too complicated
	// and I just can't figure it out. Instead, parse the url string looking for things
	// we recognize and use the "Finder folder" icon if we can't determine it.
	char* iconURL = HT_GetWorkspaceLargeIconURL(inView);

	char workspace[500];
	workspace[0] = NULL;			// clear out value left on stack from last time.
	ResIDT iconID = kWorkspaceID;
	sscanf ( iconURL, "icon/large:workspace,%s", workspace );
	if ( strcmp(workspace, "history") == 0 )
		iconID = kHistoryID;
	else if ( strcmp(workspace, "search") == 0 )
		iconID = kSearchID;
	else if ( strcmp(workspace, "sitemap") == 0 )
		iconID = kSiteMapID;
	
	workspaceImage = new TitleImage(viewName, iconID);

} // constructor

SelectorData :: ~SelectorData ( )
{
	delete workspaceImage;
}


#pragma mark -- class TitleImage --

#include "UGraphicGizmos.h"

TitleImage :: TitleImage ( const LStr255 & inTitle, ResIDT inIconID ) 
	: mTitle(inTitle), mIconID(inIconID)
{
#if DRAW_WITH_TITLE
	::TextFont(kFontIDGeneva);
	::TextSize(9);
	::TextFace(0);
	mTextWidth = ::TextWidth( inTitle, 0, inTitle.Length() ) + 6;
#endif
}


//
// DrawInCurrentView
//
// Draw a selector. The code for drawing the title has been commented out for now because
// it has not been hooked up to the text/graphics prefs.
//
void
TitleImage :: DrawInCurrentView( const Rect& inBounds, unsigned long inMode ) const
{
	StColorState saved;
	
	::RGBBackColor(&CNavCenterSelectorPane::mBGColor);
	::EraseRect(&inBounds);
	
	Rect iconRect = inBounds;
	Rect textbg = { 0, 0, 0, 0 };

#if DRAW_WITH_TITLE	
	if ( TextWidth() > inBounds.right - inBounds.left )
		textbg.right = inBounds.right - inBounds.left;
	else
		textbg.right = TextWidth();
	UGraphicGizmos::CenterRectOnRect(textbg, inBounds);
	textbg.bottom = inBounds.bottom;
	textbg.top = inBounds.bottom - 12;
#endif
	
	iconRect.right 	= iconRect.left + 16;
	iconRect.bottom	= iconRect.top  + 16;
	UGraphicGizmos::CenterRectOnRect ( iconRect, inBounds );
		
	SInt16 iconTransform = kTransformNone;
	if ( inMode & CNavCenterSelectorPane::kSelected ) {
		iconTransform = kTransformSelected;
#if DRAW_WITH_TITLE	
		::BackColor(blackColor);
		::EraseRect(&textbg);
		::TextMode(srcXor);
#endif
#if DRAW_SELECTED_AS_TAB
		Rect temp = inBounds;
		temp.left += 2;
		StRegion boundsRgn(temp);
		DrawOneTab(boundsRgn);
#endif
	}

	OSErr err = ::PlotIconID(&iconRect, kAlignNone, iconTransform, IconID());

#if DRAW_WITH_TITLE
	::MoveTo( textbg.left + 2, inBounds.bottom - 2 );
	::DrawString ( Title() );
#endif

} // DrawInCurrentView

#if DRAW_SELECTED_AS_TAB

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void TitleImage::DrawOneTabBackground(
	RgnHandle inRegion,
	Boolean inCurrentTab) const
{
	SBevelColorDesc			mActiveColors;
	
	SBevelColorDesc theTabColors;
	UGraphicGizmos::LoadBevelTraits(1000, theTabColors);
	Int16 thePaletteIndex = theTabColors.fillColor;
//	if (mTrackInside)
//		thePaletteIndex--;
	::PmForeColor(thePaletteIndex);
	::PaintRgn(inRegion);
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void TitleImage::DrawOneTabFrame(RgnHandle inRegion, Boolean inCurrentTab) const
{
	// Draw the tab frame	
	SBevelColorDesc theTabColors;
	UGraphicGizmos::LoadBevelTraits(1000, theTabColors);
	Int16 thePaletteIndex = theTabColors.frameColor;
	::PmForeColor(thePaletteIndex);
	::FrameRgn(inRegion);
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void TitleImage::DrawCurrentTabSideClip(RgnHandle inRegion) const
{
	SBevelColorDesc theTabColors;
	UGraphicGizmos::LoadBevelTraits(1000, theTabColors);
	
	// Always only for the current tab
	::SetClip(inRegion);
	::PmForeColor(theTabColors.fillColor);
//	::PaintRect(&mSideClipFrame);
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ


void TitleImage::DrawOneTab(RgnHandle inBounds) const
{
	StClipRgnState theClipSaver;

#if 0
	Rect theControlFrame;
	CalcLocalFrameRect(theControlFrame);

	Boolean isCurrentTab = (inTab == mCurrentTab);
	if (isCurrentTab)
		{
		StRegion theTempMask(inTab->mMask);
		StRegion theSideMask(mSideClipFrame);
		::DiffRgn(theTempMask, theSideMask, theTempMask);
		::SetClip(theTempMask);
		}
	else
		{
		StRegion theTempMask(inTab->mMask);
		::DiffRgn(theTempMask, mCurrentTab->mMask, theTempMask);
		::SetClip(theTempMask);
		}
#endif
	
	// This draws the fill pattern in the tab	
	DrawOneTabBackground(inBounds, true);
	// This just calls FrameRgn on inTab->mMask
//	DrawOneTabFrame(inBounds, true);

	StColorPenState thePenSaver;
	thePenSaver.Normalize();

	DrawCurrentTabSideClip(inBounds);

	Uint8 mBevelDepth = 2;
	if (mBevelDepth > 0)
	{

		::PenSize(mBevelDepth, mBevelDepth);

		bool isCurrentTab = true;
		if (isCurrentTab) {
			// Draw the top bevel		
			RGBColor	theAddColor = {0x4000, 0x4000, 0x4000};
			::RGBForeColor(&theAddColor);
			::OpColor(&UGraphicGizmos::sLighter);
			::PenMode(subPin);
			
			// This runs the pen around the top and left sides
			DrawTopBevel(inBounds);
		}

		// Draw the bevel shadow
//		StRegion theShadeRgn(inTab->mShadeFrame);
		
//		::SetClip(theShadeRgn);
		// Set up the colors for the bevel shadow
		RGBColor	theSubColor = {0x4000, 0x4000, 0x4000};
		::RGBForeColor(&theSubColor);
		::OpColor(&UGraphicGizmos::sDarker);
		::PenMode(subPin);
		// This runs the pen around the bottom and right sides
		DrawBottomBevel(inBounds, true);
	}

}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void TitleImage::DrawTopBevel(RgnHandle inBounds) const
{
	const Rect& theTabFrame = (**inBounds).rgnBBox;
	enum { eNorthTab = 0, eEastTab, eSouthTab, eWestTab };
	Uint8 mOrientation = eWestTab;
	Uint8 mCornerPixels = 5, mBevelDepth = 2;
	
	switch (mOrientation)
		{
		case eNorthTab:
			{
			::MoveTo(theTabFrame.left + 1, theTabFrame.bottom + 1);
			::LineTo(theTabFrame.left + 1, theTabFrame.top + mCornerPixels);
			::LineTo(theTabFrame.left + mCornerPixels, theTabFrame.top + 1);
			::LineTo(theTabFrame.right - (mCornerPixels + mBevelDepth) + 1, theTabFrame.top + 1);
			}
			break;

		case eEastTab:
			{
			// I'm not sure whether this is quite right...
			::MoveTo(theTabFrame.left - 1, theTabFrame.top + 1);
			::LineTo(theTabFrame.right - (mCornerPixels + mBevelDepth) + 1, theTabFrame.top + 1);
			::LineTo(theTabFrame.right - mBevelDepth, theTabFrame.top + mCornerPixels);
			}
			break;
			
		case eSouthTab:
			{
			// I'm not sure whether this is quite right...
			::MoveTo(theTabFrame.left + 1, theTabFrame.top - 1);
			::LineTo(theTabFrame.left + 1, theTabFrame.bottom - (mCornerPixels + mBevelDepth) + 1);
			::LineTo(theTabFrame.left + mCornerPixels, theTabFrame.bottom - mBevelDepth);
			}
			break;
		
		case eWestTab:
			{
			::MoveTo(theTabFrame.right + 1, theTabFrame.top + 1);
			::LineTo(theTabFrame.left + mCornerPixels, theTabFrame.top + 1);
			::LineTo(theTabFrame.left + 1, theTabFrame.top + mCornerPixels);
			::LineTo(theTabFrame.left + 1, theTabFrame.bottom - (mCornerPixels + mBevelDepth) + 1);
			}
			break;
		}
}

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	₯	DrawBottomBevel
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

void TitleImage::DrawBottomBevel(RgnHandle inBounds, Boolean	inCurrentTab) const
{
	const Rect& theTabFrame = (**inBounds).rgnBBox;
	enum { eNorthTab = 0, eEastTab, eSouthTab, eWestTab };
	Uint8 mOrientation = eWestTab;
	Uint8 mCornerPixels = 5, mBevelDepth = 2;

	switch (mOrientation)
		{
		case eNorthTab:
			{
			::MoveTo(theTabFrame.right - mBevelDepth, theTabFrame.top + mCornerPixels);
			::LineTo(theTabFrame.right - mBevelDepth, theTabFrame.bottom + 1);
			}
			break;

		case eEastTab:
			{
			// I'm not sure whether this is quite right...
			::MoveTo(theTabFrame.right - mBevelDepth, theTabFrame.top + mCornerPixels);			
			::LineTo(theTabFrame.right - mBevelDepth, theTabFrame.bottom - (mCornerPixels + mBevelDepth) + 1);
			::LineTo(theTabFrame.right - (mCornerPixels + mBevelDepth) + 1, theTabFrame.bottom - mBevelDepth);
			::LineTo(theTabFrame.left - 1, theTabFrame.bottom - mBevelDepth);
			}
			break;
			
		case eSouthTab:
			{
			// I'm not sure whether this is quite right...
			::MoveTo(theTabFrame.left + mCornerPixels, theTabFrame.bottom - mBevelDepth);
			::LineTo(theTabFrame.right - (mCornerPixels + mBevelDepth) + 1, theTabFrame.bottom - mBevelDepth);
			::LineTo(theTabFrame.right - mBevelDepth, theTabFrame.bottom - (mCornerPixels + mBevelDepth) + 1);
			::LineTo(theTabFrame.right - mBevelDepth, theTabFrame.top - 1);
			}
			break;
		
		case eWestTab:
			{
			::MoveTo(theTabFrame.left + mCornerPixels, theTabFrame.bottom - mBevelDepth);
			::LineTo(theTabFrame.right + 1, theTabFrame.bottom - mBevelDepth);
			}
			break;
		}
}

#endif