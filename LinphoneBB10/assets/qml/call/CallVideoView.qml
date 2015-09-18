/*
 * CallVideo.qml
 * Copyright (C) 2015  Belledonne Communications, Grenoble, France
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

import bb.cascades 1.4

import "../custom_controls"

Container {
    layout: DockLayout {

    }
    layoutProperties: StackLayoutProperties {
        spaceQuota: 1
    }
    horizontalAlignment: HorizontalAlignment.Fill

    ForeignWindowControl {
        windowId: "LinphoneVideoWindowId" // Do not change the name of this windowId
        visible: boundToWindow // becomes visible once bound to a window
        updatedProperties: WindowProperty.Position | WindowProperty.Size
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill

        onCreationCompleted: {
            inCallModel.onVideoSurfaceCreationCompleted(windowId, windowGroup);
        }

        gestureHandlers: TapHandler {
            onTapped: {
                inCallModel.switchFullScreenMode();
            }
        }
    }

    ForeignWindowControl {
        windowId: "LinphoneLocalVideoWindowId" // Do not change the name of this windowId
        visible: settingsModel.previewVisible && boundToWindow // becomes visible once bound to a window
        updatedProperties: WindowProperty.Position | WindowProperty.Size | WindowProperty.Visible
        horizontalAlignment: HorizontalAlignment.Right
        verticalAlignment: VerticalAlignment.Bottom
        preferredWidth: inCallModel.previewSize.width
        preferredHeight: inCallModel.previewSize.height

        onWindowAttached: {
            inCallModel.cameraPreviewAttached(windowHandle)
        }
    }

    Container {
        layout: StackLayout {
            orientation: LayoutOrientation.TopToBottom
        }
        horizontalAlignment: HorizontalAlignment.Fill

        Container {
            layout: DockLayout {

            }
            verticalAlignment: VerticalAlignment.Top
            horizontalAlignment: HorizontalAlignment.Fill
            minHeight: ui.sdu(20)
            maxHeight: ui.sdu(20)
            visible: inCallModel.areControlsVisible

            Container {
                background: colors.colorH
                opacity: 0.7
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
            }

            Container {
                layout: StackLayout {
                    orientation: LayoutOrientation.TopToBottom
                }
                verticalAlignment: VerticalAlignment.Center
                horizontalAlignment: HorizontalAlignment.Center

                Label {
                    text: inCallModel.displayName
                    horizontalAlignment: HorizontalAlignment.Center
                    textStyle.fontSize: FontSize.XLarge
                    textStyle.color: colors.colorC
                    textStyle.base: titilliumWeb.style
                }

                Label {
                    text: inCallModel.callTime
                    horizontalAlignment: HorizontalAlignment.Center
                    textStyle.color: colors.colorA
                    textStyle.base: titilliumWeb.style
                }
            }
        }

        Container {
            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }
            leftPadding: ui.sdu(5)
            rightPadding: ui.sdu(5)
            topPadding: ui.sdu(2)
            visible: inCallModel.areControlsVisible

            ImageButton {
                defaultImageSource: "asset:///images/call/camera_switch_default.png"
                pressedImageSource: "asset:///images/call/camera_switch_over.png"
                disabledImageSource: "asset:///images/call/camera_switch_disabled.png"
                verticalAlignment: VerticalAlignment.Center
                horizontalAlignment: HorizontalAlignment.Center

                onClicked: {
                    inCallModel.switchCamera();
                }
            }

            Container {
                layoutProperties: StackLayoutProperties {
                    spaceQuota: 1
                }
            }

            CustomImageToggle {
                imageSource: "asset:///images/call/pause_big_default.png"
                selectedImageSource: "asset:///images/call/pause_big_over_selected.png"
                verticalAlignment: VerticalAlignment.Center
                horizontalAlignment: HorizontalAlignment.Center
                selected: inCallModel.isPaused

                gestureHandlers: TapHandler {
                    onTapped: {
                        inCallModel.togglePause();
                    }
                }
            }
        }
    }
}