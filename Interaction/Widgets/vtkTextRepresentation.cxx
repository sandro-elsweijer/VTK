/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextRepresentation.h"

#include "vtkCommand.h"
#include "vtkMathTextFreeTypeTextRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkStdString.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"
#include "vtkWindow.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkTextRepresentationObserver : public vtkCommand
{
public:
  static vtkTextRepresentationObserver* New() { return new vtkTextRepresentationObserver; }

  void SetTarget(vtkTextRepresentation* t) { this->Target = t; }
  void Execute(vtkObject* o, unsigned long event, void* p) override
  {
    if (this->Target)
    {
      if (o && vtkTextActor::SafeDownCast(o))
      {
        this->Target->ExecuteTextActorModifiedEvent(o, event, p);
      }
      else if (o && vtkTextProperty::SafeDownCast(o))
      {
        this->Target->ExecuteTextPropertyModifiedEvent(o, event, p);
      }
    }
  }

protected:
  vtkTextRepresentationObserver() { this->Target = nullptr; }
  vtkTextRepresentation* Target;
};

vtkStandardNewMacro(vtkTextRepresentation);

//------------------------------------------------------------------------------
vtkTextRepresentation::vtkTextRepresentation()
{
  this->Observer = vtkTextRepresentationObserver::New();
  this->Observer->SetTarget(this);

  this->TextActor = vtkTextActor::New();
  this->InitializeTextActor();

  this->SetShowBorderToActive();
  this->BWActorEdges->VisibilityOff();
}

//------------------------------------------------------------------------------
vtkTextRepresentation::~vtkTextRepresentation()
{
  this->SetTextActor(nullptr);
  this->Observer->SetTarget(nullptr);
  this->Observer->Delete();
}

//------------------------------------------------------------------------------
void vtkTextRepresentation::SetTextActor(vtkTextActor* textActor)
{
  if (textActor != this->TextActor)
  {
    if (this->TextActor)
    {
      this->TextActor->GetTextProperty()->RemoveObserver(this->Observer);
      this->TextActor->RemoveObserver(this->Observer);
      this->TextActor->Delete();
    }
    this->TextActor = textActor;
    if (this->TextActor)
    {
      this->TextActor->Register(this);
    }

    this->InitializeTextActor();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkTextRepresentation::SetText(const char* text)
{
  if (this->TextActor)
  {
    this->TextActor->SetInput(text);
  }
  else
  {
    vtkErrorMacro("No Text Actor present. Cannot set text.");
  }
}

//------------------------------------------------------------------------------
const char* vtkTextRepresentation::GetText()
{
  if (this->TextActor)
  {
    return this->TextActor->GetInput();
  }
  vtkErrorMacro("No text actor present. No showing any text.");
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkTextRepresentation::BuildRepresentation()
{
  // Ask the superclass the size and set the text
  double* pos1 = this->PositionCoordinate->GetComputedDoubleDisplayValue(this->Renderer);
  double* pos2 = this->Position2Coordinate->GetComputedDoubleDisplayValue(this->Renderer);

  if (this->TextActor)
  {
    // Add the padding when setting the position of the Text
    const double textPos1[] = { pos1[0] + this->PaddingLeft, pos1[1] + this->PaddingBottom };

    const double textPos2[] = { pos2[0] - this->PaddingRight, pos2[1] - this->PaddingTop };

    this->TextActor->GetPositionCoordinate()->SetValue(textPos1[0], textPos1[1]);
    this->TextActor->GetPosition2Coordinate()->SetValue(textPos2[0], textPos2[1]);
  }

  // Note that the transform is updated by the superclass
  this->Superclass::BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkTextRepresentation::GetActors2D(vtkPropCollection* pc)
{
  pc->AddItem(this->TextActor);
  this->Superclass::GetActors2D(pc);
}

//------------------------------------------------------------------------------
void vtkTextRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->TextActor->ReleaseGraphicsResources(w);
  this->Superclass::ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
int vtkTextRepresentation::RenderOverlay(vtkViewport* w)
{
  int count = this->Superclass::RenderOverlay(w);
  count += this->TextActor->RenderOverlay(w);
  return count;
}

//------------------------------------------------------------------------------
int vtkTextRepresentation::RenderOpaqueGeometry(vtkViewport* w)
{
  // CheckTextBoundary resize the text actor. This needs to happen before we
  // actually render (previous version was calling this method after
  // this->TextActor->RenderOpaqueGeometry(), which seems like a bug).
  this->CheckTextBoundary();
  int count = this->Superclass::RenderOpaqueGeometry(w);
  count += this->TextActor->RenderOpaqueGeometry(w);
  return count;
}

//------------------------------------------------------------------------------
int vtkTextRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* w)
{
  int count = this->Superclass::RenderTranslucentPolygonalGeometry(w);
  count += this->TextActor->RenderTranslucentPolygonalGeometry(w);
  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkTextRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = this->Superclass::HasTranslucentPolygonalGeometry();
  result |= this->TextActor->HasTranslucentPolygonalGeometry();
  return result;
}

//------------------------------------------------------------------------------
void vtkTextRepresentation::InitializeTextActor()
{
  if (this->TextActor)
  {
    this->TextActor->SetTextScaleModeToProp();
    this->TextActor->SetMinimumSize(1, 1);
    this->TextActor->SetMaximumLineHeight(1.0);
    this->TextActor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
    this->TextActor->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
    this->TextActor->GetPosition2Coordinate()->SetReferenceCoordinate(nullptr);
    this->TextActor->GetTextProperty()->SetJustificationToCentered();
    this->TextActor->GetTextProperty()->SetVerticalJustificationToCentered();

    this->TextActor->UseBorderAlignOn();

    this->TextProperty = this->TextActor->GetTextProperty();

    this->TextActor->GetTextProperty()->AddObserver(vtkCommand::ModifiedEvent, this->Observer);
    this->TextActor->AddObserver(vtkCommand::ModifiedEvent, this->Observer);
  }
}

//------------------------------------------------------------------------------
void vtkTextRepresentation::ExecuteTextPropertyModifiedEvent(
  vtkObject* object, unsigned long enumEvent, void*)
{
  if (!object || enumEvent != vtkCommand::ModifiedEvent)
  {
    return;
  }
  vtkTextProperty* tp = vtkTextProperty::SafeDownCast(object);
  if (!tp)
  {
    return;
  }

  this->CheckTextBoundary();
}

//------------------------------------------------------------------------------
void vtkTextRepresentation::ExecuteTextActorModifiedEvent(
  vtkObject* object, unsigned long enumEvent, void*)
{
  if (!object || enumEvent != vtkCommand::ModifiedEvent)
  {
    return;
  }
  vtkTextActor* ta = vtkTextActor::SafeDownCast(object);
  if (!ta || ta != this->TextActor)
  {
    return;
  }

  if (this->TextProperty != this->TextActor->GetTextProperty())
  {
    this->TextActor->GetTextProperty()->AddObserver(vtkCommand::ModifiedEvent, this->Observer);
    this->TextProperty = this->TextActor->GetTextProperty();
  }

  this->CheckTextBoundary();
}

//------------------------------------------------------------------------------
void vtkTextRepresentation::CheckTextBoundary()
{
  if (this->GetRenderer() &&
    this->TextActor->GetTextScaleMode() != vtkTextActor::TEXT_SCALE_MODE_PROP)
  {
    vtkTextRenderer* tren = vtkTextRenderer::GetInstance();
    if (!tren)
    {
      vtkErrorMacro(<< "Failed getting the vtkTextRenderer instance");
      return;
    }

    this->TextActor->ComputeScaledFont(this->GetRenderer());

    vtkWindow* win = this->Renderer->GetVTKWindow();
    if (!win)
    {
      vtkErrorMacro(<< "No render window available: cannot determine DPI.");
      return;
    }

    int text_bbox[4];
    if (!tren->GetBoundingBox(
          this->TextActor->GetScaledTextProperty(), this->GetText(), text_bbox, win->GetDPI()))
    {
      return;
    }

    // The bounding box was the area that is going to be filled with pixels
    // given a text origin of (0, 0). Now get the real size we need, i.e.
    // the full extent from the origin to the bounding box.
    double text_size[] = { (text_bbox[1] - text_bbox[0] + 1.0),
      (text_bbox[3] - text_bbox[2] + 1.0) };

    this->GetRenderer()->DisplayToNormalizedDisplay(text_size[0], text_size[1]);
    this->GetRenderer()->NormalizedDisplayToViewport(text_size[0], text_size[1]);
    this->GetRenderer()->ViewportToNormalizedViewport(text_size[0], text_size[1]);

    // Convert padding in pixels into viewport
    // Multiply by 2 to ensure an even padding
    int* size = win->GetSize();
    double paddingX = (this->PaddingLeft + this->PaddingRight) / (double)size[0];
    double paddingY = (this->PaddingTop + this->PaddingBottom) / (double)size[1];

    double posX = text_size[0] + paddingX;
    double posY = text_size[1] + paddingY;

    // update the PositionCoordinate and add padding
    double* pos2 = this->Position2Coordinate->GetValue();
    if (pos2[0] != posX || pos2[1] != posY)
    {
      this->Position2Coordinate->SetValue(posX, posY, 0);
      this->Modified();
    }
    if (this->WindowLocation != vtkBorderRepresentation::AnyLocation)
    {
      this->UpdateWindowLocation();
    }
  }
}

//------------------------------------------------------------------------------
void vtkTextRepresentation::SetWindowLocation(int enumLocation)
{
  if (this->WindowLocation != enumLocation)
  {
    this->WindowLocation = enumLocation;
    this->CheckTextBoundary();
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkTextRepresentation::SetPosition(double x, double y)
{
  double* pos = this->PositionCoordinate->GetValue();
  if (pos[0] == x && pos[1] == y)
  {
    return;
  }

  this->PositionCoordinate->SetValue(x, y);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkTextRepresentation::SetPadding(int padding)
{
  padding = std::max(0, std::min(4000, padding));

  // Negative padding does not make sense
  this->PaddingLeft = padding;
  this->PaddingRight = padding;
  this->PaddingTop = padding;
  this->PaddingBottom = padding;
}

//------------------------------------------------------------------------------
void vtkTextRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Text Actor: " << this->TextActor << "\n";
}
VTK_ABI_NAMESPACE_END
