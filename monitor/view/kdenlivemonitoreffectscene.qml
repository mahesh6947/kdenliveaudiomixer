import QtQuick 2.0

Item {
    id: root
    objectName: "rooteffectscene"
    SystemPalette { id: activePalette }

    // default size, but scalable by user
    height: 300; width: 400
    property string comment
    property string framenum
    property rect framesize
    property rect adjustedFrame
    property point profile
    property point center
    property double zoom
    property double scalex
    property double scaley
    property double offsetx : 0
    property double offsety : 0
    property double lockratio : -1
    property double timeScale: 1
    property double frameSize: 10
    property int duration: 300
    property bool mouseOverRuler: false
    property int mouseRulerPos: 0
    onScalexChanged: canvas.requestPaint()
    onScaleyChanged: canvas.requestPaint()
    onOffsetxChanged: canvas.requestPaint()
    onOffsetyChanged: canvas.requestPaint()
    property bool iskeyframe
    property int requestedKeyFrame
    property var centerPoints: []
    property var centerPointsTypes: []
    onCenterPointsChanged: canvas.requestPaint()
    property bool showToolbar: false
    signal effectChanged()
    signal centersChanged()
    signal addKeyframe()
    signal seekToKeyframe()
    signal toolBarChanged(bool doAccept)
    onZoomChanged: {
        effectToolBar.setZoom(root.zoom)
    }
    onDurationChanged: {
        timeScale = width / duration
        if (duration < 200) {
            frameSize = 5 * timeScale
        } else if (duration < 2500) {
            frameSize = 25 * timeScale
        } else if (duration < 10000) {
            frameSize = 50 * timeScale
        } else {
            frameSize = 100 * timeScale
        }
    }

    Text {
        id: fontReference
        property int fontSize
        fontSize: font.pointSize
    }

    Canvas {
      id: canvas
      property double handleSize
      width: root.width
      height: root.height
      anchors.centerIn: root
      contextType: "2d";
      handleSize: fontReference.fontSize / 2
      renderStrategy: Canvas.Threaded;
      onPaint:
      {
        if (context && root.centerPoints.length > 0) {
            context.beginPath()
            context.strokeStyle = Qt.rgba(1, 0, 0, 0.5)
            context.fillStyle = Qt.rgba(1, 0, 0, 0.5)
            context.lineWidth = 2
            var p1 = convertPoint(root.centerPoints[0])
            context.moveTo(p1.x, p1.y)
            context.clearRect(0,0, width, height);
            context.fillRect(p1.x - handleSize, p1.y - handleSize, 2 * handleSize, 2 * handleSize);
            for(var i = 0; i < root.centerPoints.length; i++)
            {
                context.fillRect(p1.x - handleSize, p1.y - handleSize, 2 * handleSize, 2 * handleSize);
                if (i + 1 < root.centerPoints.length)
                {
                    var end = convertPoint(root.centerPoints[i + 1])

                    if (root.centerPointsTypes.length != root.centerPoints.length || root.centerPointsTypes[i] == 0) {
                        context.lineTo(end.x, end.y)
                        p1 = end
                        continue
                    }

                    var j = i - 1
                    if (j < 0) {
                        j = 0
                    }
                    var pre = convertPoint(root.centerPoints[j])
                    j = i + 2
                    if (j >= root.centerPoints.length) {
                        j = root.centerPoints.length - 1
                    }
                    var post = convertPoint(root.centerPoints[j])
                    var c1 = substractPoints(end, pre, 6.0)
                    var c2 = substractPoints(p1, post, 6.0)
                    var mid = distance(end, p1) / 2
                    var c1Dist = Math.sqrt(Math.pow(c1.x, 2) + Math.pow(c1.y, 2))
                    if (c1Dist > mid) {
                        c1.x = c1.x * mid / c1Dist
                        c1.y = c1.y * mid / c1Dist
                    }
                    var c2Dist = Math.sqrt(Math.pow(c2.x, 2) + Math.pow(c2.y, 2))
                    if (c2Dist > mid) {
                        c2.x = c2.x * -mid / c2Dist
                        c2.y = c2.y * -mid / c2Dist
                    }
                    c1 = addPoints(c1, p1)
                    c2 = addPoints(c2, end)
                    context.bezierCurveTo(c1.x, c1.y, c2.x, c2.y, end.x, end.y);
                    //context.lineTo(end.x, end.y);
                } else {
                    context.lineTo(p1.x, p1.y)
                }
                p1 = end
            }
            context.stroke()
            context.restore()
        }
    }

    function convertPoint(p)
    {
        var x = frame.x + p.x * root.scalex
        var y = frame.y + p.y * root.scaley
        return Qt.point(x,y);
    }
    function addPoints(p1, p2)
    {
        var x = p1.x + p2.x
        var y = p1.y + p2.y
        return Qt.point(x,y);
    }
    function distance(p1, p2)
    {
        var x = p1.x + p2.x
        var y = p1.y + p2.y
        return Math.sqrt(Math.pow(p2.x - p1.x, 2) + Math.pow(p2.y - p1.y, 2));
    }
    function substractPoints(p1, p2, f)
    {
        var x = (p1.x - p2.x) / f
        var y = (p1.y - p2.y) / f
        return Qt.point(x,y);
    }
  }
    Rectangle {
        id: frame
        objectName: "referenceframe"
        property color hoverColor: "#ff0000"
        width: root.profile.x * root.scalex
        height: root.profile.y * root.scaley
        x: root.center.x - width / 2 - root.offsetx;
        y: root.center.y - height / 2 - root.offsety;
        color: "transparent"
        border.color: "#ffffff00"
    }
    MouseArea {
        id: global
        objectName: "global"
        width: root.width; height: root.height
        property bool isMoving : false
        anchors.centerIn: root
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor

        readonly property bool containsMouse: {
              if (isMoving) {
                  return true;
              }
              for(var i = 0; i < root.centerPoints.length; i++)
              {
                var p1 = canvas.convertPoint(root.centerPoints[i])
                if (Math.abs(p1.x - mouseX) <= canvas.handleSize && Math.abs(p1.y - mouseY) <= canvas.handleSize) {
                    root.requestedKeyFrame = i
                    return true
                }
              }
              root.requestedKeyFrame = -1
              return false
        }

        onPositionChanged: {
            if (!pressed) {
                mouse.accepted = false
                return
            }
            if (root.requestedKeyFrame != -1) {
                  isMoving = true
                  root.centerPoints[root.requestedKeyFrame].x = (mouseX - frame.x) / root.scalex;
                  root.centerPoints[root.requestedKeyFrame].y = (mouseY - frame.y) / root.scaley;
                  canvas.requestPaint()
                  root.centersChanged()
            }
        }

        onPressed: {
            if (mouse.button & Qt.LeftButton) {
                if (root.requestedKeyFrame >= 0 && !isMoving) {
                    root.seekToKeyframe();
                }
            }
            isMoving = false

        }
        onDoubleClicked: {
            root.addKeyframe()
        }
        onReleased: {
            root.requestedKeyFrame = -1
            isMoving = false;
        }
    }
    EffectToolBar {
        id: effectToolBar
        anchors {
            left: parent.left
            top: parent.top
            topMargin: 10
            leftMargin: 10
        }
        visible: root.showToolbar
    }
    Rectangle {
        id: framerect
        property color hoverColor: "#ffffff"
        x: frame.x + root.framesize.x * root.scalex
        y: frame.y + root.framesize.y * root.scaley
        width: root.framesize.width * root.scalex
        height: root.framesize.height * root.scaley
        color: "transparent"
        border.color: "#ffff0000"
        Rectangle {
            id: tlhandle
            anchors {
            top: parent.top
            left: parent.left
            }
            visible: root.iskeyframe
            width: effectsize.height * 0.7
            height: this.width
            color: "red"
            MouseArea {
              property int oldMouseX
              property int oldMouseY
              width: parent.width; height: parent.height
              anchors.centerIn: parent
              hoverEnabled: true
              cursorShape: Qt.SizeFDiagCursor
              onEntered: { 
                if (!pressed) {
                  tlhandle.color = '#ffff00'
                }
              }
              onExited: {
                if (!pressed) {
                  tlhandle.color = '#ff0000'
                }
              }
              onPressed: {
                  oldMouseX = mouseX
                  oldMouseY = mouseY
                  effectsize.visible = true
                  tlhandle.color = '#ffff00'
              }
              onPositionChanged: {
                if (pressed) {
                  if (root.lockratio > 0) {
                      var delta = Math.max(mouseX - oldMouseX, mouseY - oldMouseY)
                      var newwidth = framerect.width - delta
                      adjustedFrame = framesize
                      adjustedFrame.width = Math.round(newwidth / root.scalex);
                      adjustedFrame.height = Math.round(adjustedFrame.width / root.lockratio)
                      adjustedFrame.y = (framerect.y - frame.y) / root.scaley + framesize.height - adjustedFrame.height;
                      adjustedFrame.x = (framerect.x - frame.x) / root.scalex + framesize.width - adjustedFrame.width;
                      framesize = adjustedFrame
                  } else {
                    framesize.x = (framerect.x + (mouseX - oldMouseX) - frame.x) / root.scalex;
                    framesize.width = (framerect.width - (mouseX - oldMouseX)) / root.scalex;
                    framesize.y = (framerect.y + (mouseY - oldMouseY) - frame.y) / root.scaley;
                    framesize.height = (framerect.height - (mouseY - oldMouseY)) / root.scaley;
                  }
                  root.effectChanged()
                }
              }
              onReleased: {
                  effectsize.visible = false
                  tlhandle.color = '#ff0000'
              }
            }
            Text {
                id: effectpos
                objectName: "effectpos"
                color: "red"
                visible: false
                anchors {
                    top: parent.bottom
                    left: parent.right
                }
                text: framesize.x.toFixed(0) + "x" + framesize.y.toFixed(0)
            }
        }
        Rectangle {
            id: trhandle
            anchors {
            top: parent.top
            right: parent.right
            }
            width: effectsize.height * 0.7
            height: this.width
            color: "red"
            visible: root.iskeyframe
            MouseArea {
              property int oldMouseX
              property int oldMouseY
              width: parent.width; height: parent.height
              anchors.centerIn: parent
              hoverEnabled: true
              cursorShape: Qt.SizeBDiagCursor
              onEntered: {
                if (!pressed) {
                  trhandle.color = '#ffff00'
                }
              }
              onExited: {
                if (!pressed) {
                  trhandle.color = '#ff0000'
                }
              }
              onPressed: {
                  oldMouseX = mouseX
                  oldMouseY = mouseY
                  effectsize.visible = true
                  trhandle.color = '#ffff00'
              }
              onPositionChanged: {
                if (pressed) {
                  if (root.lockratio > 0) {
                      var delta = Math.max(oldMouseX - mouseX, mouseY - oldMouseY)
                      var newwidth = framerect.width - delta
                      adjustedFrame = framesize
                      adjustedFrame.width = Math.round(newwidth / root.scalex);
                      adjustedFrame.height = Math.round(adjustedFrame.width / root.lockratio)
                      adjustedFrame.y = (framerect.y - frame.y) / root.scaley + framesize.height - adjustedFrame.height;
                      framesize = adjustedFrame
                  } else {
                      framesize.width = (framerect.width + (mouseX - oldMouseX)) / root.scalex;
                      framesize.y = (framerect.y + (mouseY - oldMouseY) - frame.y) / root.scaley;
                      framesize.height = (framerect.height - (mouseY - oldMouseY)) / root.scaley;
                  }
                  root.effectChanged()
                }
              }
              onReleased: {
                  effectsize.visible = false
                  trhandle.color = '#ff0000'
              }
            }
        }
        Rectangle {
            id: blhandle
            anchors {
            bottom: parent.bottom
            left: parent.left
            }
            width: effectsize.height * 0.7
            height: this.width
            color: "red"
            visible: root.iskeyframe
            MouseArea {
              property int oldMouseX
              property int oldMouseY
              width: parent.width; height: parent.height
              anchors.centerIn: parent
              hoverEnabled: true
              cursorShape: Qt.SizeBDiagCursor
              onEntered: {
                if (!pressed) {
                  blhandle.color = '#ffff00'
                }
              }
              onExited: {
                if (!pressed) {
                  blhandle.color = '#ff0000'
                }
              }
              onPressed: {
                  oldMouseX = mouseX
                  oldMouseY = mouseY
                  effectsize.visible = true
                  blhandle.color = '#ffff00'
              }
              onPositionChanged: {
                if (pressed) {
                  if (root.lockratio > 0) {
                      var delta = Math.max(mouseX - oldMouseX, oldMouseY - mouseY)
                      var newwidth = framerect.width - delta
                      framesize.x = (framerect.x + (framerect.width - newwidth) - frame.x) / root.scalex;
                      framesize.width = Math.round(newwidth / root.scalex);
                      framesize.height = Math.round(framesize.width / root.lockratio)
                  } else {
                    framesize.x = (framerect.x + (mouseX - oldMouseX) - frame.x) / root.scalex;
                    framesize.width = (framerect.width - (mouseX - oldMouseX)) / root.scalex;
                    framesize.height = (framerect.height + (mouseY - oldMouseY)) / root.scaley;
                  }
                  root.effectChanged()
                }
              }
              onReleased: {
                  effectsize.visible = false
                  blhandle.color = '#ff0000'
              }
            }
        }
        Rectangle {
            id: brhandle
            anchors {
            bottom: parent.bottom
            right: parent.right
            }
            width: effectsize.height * 0.7
            height: this.width
            color: "red"
            visible: root.iskeyframe
            MouseArea {
              property int oldMouseX
              property int oldMouseY
              width: parent.width; height: parent.height
              anchors.centerIn: parent
              hoverEnabled: true
              cursorShape: Qt.SizeFDiagCursor
              onEntered: {
                if (!pressed) {
                  brhandle.color = '#ffff00'
                }
              }
              onExited: {
                if (!pressed) {
                  brhandle.color = '#ff0000'
                }
              }
              onPressed: {
                  oldMouseX = mouseX
                  oldMouseY = mouseY
                  effectsize.visible = true
                  brhandle.color = '#ffff00'
              }
              onPositionChanged: {
                if (pressed) {
                   if (root.lockratio > 0) {
                      var delta = Math.max(oldMouseX - mouseX, oldMouseY - mouseY)
                      var newwidth = framerect.width - delta
                      framesize.width = Math.round(newwidth / root.scalex);
                      framesize.height = Math.round(framesize.width / root.lockratio)
                  } else {
                    framesize.width = (framerect.width + (mouseX - oldMouseX)) / root.scalex;
                    framesize.height = (framerect.height + (mouseY - oldMouseY)) / root.scaley;
                  }
                  root.effectChanged()
                }
              }
              onReleased: {
                  effectsize.visible = false
                  brhandle.color = '#ff0000'
              }
            }
            Text {
                id: effectsize
                objectName: "effectsize"
                color: "red"
                visible: false
                anchors {
                    bottom: parent.top
                    right: parent.left
                }
                text: framesize.width.toFixed(0) + "x" + framesize.height.toFixed(0)
            }
        }
        Rectangle {
            anchors.centerIn: parent
            width: 1
            height: root.iskeyframe ? effectsize.height * 1.5 : effectsize.height / 2
            color: framerect.hoverColor
            MouseArea {
              width: effectsize.height * 1.5; height: effectsize.height * 1.5
              anchors.centerIn: parent
              property int oldMouseX
              property int oldMouseY
              hoverEnabled: true
              enabled: root.iskeyframe
              cursorShape: root.iskeyframe ? Qt.SizeAllCursor : Qt.ArrowCursor
              onEntered: { framerect.hoverColor = '#ffff00'}
              onExited: { framerect.hoverColor = '#ffffff'}
              onPressed: {
                  oldMouseX = mouseX
                  oldMouseY = mouseY
                  effectpos.visible = true
              }
              onPositionChanged: {
                  if (pressed) {
                      framesize.x = (framerect.x + (mouseX - oldMouseX) - frame.x) / root.scalex;
                      framesize.y = (framerect.y + (mouseY - oldMouseY) - frame.y) / root.scaley;
                      root.effectChanged()
                  }
              }
              onReleased: {
                  effectpos.visible = false
              }
            }
        }
        Rectangle {
            anchors.centerIn: parent
            width: root.iskeyframe ? effectsize.height * 1.5 : effectsize.height / 2
            height: 1
            color: framerect.hoverColor
        }
    }
    MonitorRuler {
        id: clipMonitorRuler
        anchors {
            left: root.left
            right: root.right
            bottom: root.bottom
        }
        height: controller.rulerHeight
    }
}
