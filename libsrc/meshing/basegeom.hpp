#ifndef FILE_BASEGEOM
#define FILE_BASEGEOM

/**************************************************************************/
/* File:   basegeom.hpp                                                   */
/* Author: Joachim Schoeberl                                              */
/* Date:   23. Aug. 09                                                    */
/**************************************************************************/


struct Tcl_Interp;

namespace netgen
{
  class GeometryVertex
  {
  public:
    virtual ~GeometryVertex() {}
    virtual Point<3> GetPoint() const = 0;
    virtual size_t GetHash() const = 0;
  };

  class GeometryEdge
  {
  public:
    virtual ~GeometryEdge() {}
    virtual const GeometryVertex& GetStartVertex() const = 0;
    virtual const GeometryVertex& GetEndVertex() const = 0;
    virtual double GetLength() const = 0;
    virtual Point<3> GetPoint(double t) const = 0;
    // Calculate parameter step respecting edges sag value
    virtual double CalcStep(double t, double sag) const = 0;
    virtual bool OrientedLikeGlobal() const = 0;
    virtual size_t GetHash() const = 0;
    virtual Array<Point<3>> GetEquidistantPointArray(size_t npoints) const;
  };

  class GeometryFace
  {
  public:
    virtual ~GeometryFace() {}
    virtual size_t GetNBoundaries() const = 0;
    virtual Array<unique_ptr<GeometryEdge>> GetBoundary(size_t index) const = 0;
    virtual string GetName() const { return "default"; }
    // Project point using geo info. Fast if point is close to
    // parametrization in geo info.
    virtual bool ProjectPointGI(Point<3>& p, PointGeomInfo& gi) const =0;
    virtual Point<3> GetPoint(const PointGeomInfo& gi) const = 0;
    virtual void CalcEdgePointGI(const GeometryEdge& edge,
                                 double t,
                                 EdgePointGeomInfo& egi) const = 0;
    virtual Box<3> GetBoundingBox() const = 0;

    // Get curvature in point from local coordinates in PointGeomInfo
    virtual double GetCurvature(const PointGeomInfo& gi) const = 0;

    virtual void RestrictH(Mesh& mesh, const MeshingParameters& mparam) const = 0;

  protected:
    void RestrictHTrig(Mesh& mesh,
                       const PointGeomInfo& gi0,
                       const PointGeomInfo& gi1,
                       const PointGeomInfo& gi2,
                       const MeshingParameters& mparam,
                       int depth = 0, double h = 0.) const;
  };

  class DLL_HEADER NetgenGeometry
  {
    unique_ptr<Refinement> ref;
  protected:
    Array<unique_ptr<GeometryVertex>> vertices;
    Array<unique_ptr<GeometryEdge>> edges;
    Array<unique_ptr<GeometryFace>> faces;
    Box<3> bounding_box;
  public:
    NetgenGeometry()
    {
      ref = make_unique<Refinement>(*this);
    }
    virtual ~NetgenGeometry () { ; }

    virtual int GenerateMesh (shared_ptr<Mesh> & mesh, MeshingParameters & mparam);

    virtual const Refinement & GetRefinement () const
    {
      return *ref;
    }

    virtual void DoArchive(Archive&)
  { throw NgException("DoArchive not implemented for " + Demangle(typeid(*this).name())); }

    virtual Mesh::GEOM_TYPE GetGeomType() const { return Mesh::NO_GEOM; }
    virtual void Analyse(Mesh& mesh,
                         const MeshingParameters& mparam) const;
    virtual void FindEdges(Mesh& mesh, const MeshingParameters& mparam) const;
    virtual void MeshSurface(Mesh& mesh, const MeshingParameters& mparam) const;
    virtual void OptimizeSurface(Mesh& mesh, const MeshingParameters& mparam) const;

    virtual void FinalizeMesh(Mesh& mesh) const {}

    virtual void ProjectPoint (int surfind, Point<3> & p) const
    { }
    virtual void ProjectPointEdge (int surfind, int surfind2, Point<3> & p) const { }
  virtual void ProjectPointEdge (int surfind, int surfind2, Point<3> & p, EdgePointGeomInfo& gi) const
  { ProjectPointEdge(surfind, surfind2, p); }

    virtual bool CalcPointGeomInfo(int surfind, PointGeomInfo& gi, const Point<3> & p3) const {return false;}
    virtual bool ProjectPointGI (int surfind, Point<3> & p, PointGeomInfo & gi) const
    {
      throw Exception("ProjectPointGI not overloaded in class" + Demangle(typeid(*this).name()));
    }

    virtual Vec<3> GetNormal(int surfind, const Point<3> & p) const
    { return {0.,0.,1.}; }
    virtual Vec<3> GetNormal(int surfind, const Point<3> & p, const PointGeomInfo & gi) const
    { return GetNormal(surfind, p); }
    [[deprecated]]
    void GetNormal(int surfind, const Point<3> & p, Vec<3> & n) const
    {
      n = GetNormal(surfind, p);
    }

    virtual void PointBetween (const Point<3> & p1,
                               const Point<3> & p2, double secpoint,
                               int surfi,
                               const PointGeomInfo & gi1,
                               const PointGeomInfo & gi2,
                               Point<3> & newp,
                               PointGeomInfo & newgi) const
    {
      newp = p1 + secpoint * (p2-p1);
    }

    virtual void PointBetweenEdge(const Point<3> & p1,
                                  const Point<3> & p2, double secpoint,
                                  int surfi1, int surfi2,
                                  const EdgePointGeomInfo & ap1,
                                  const EdgePointGeomInfo & ap2,
                                  Point<3> & newp,
                                  EdgePointGeomInfo & newgi) const
    {
      newp = p1+secpoint*(p2-p1);
    }

    virtual Vec<3> GetTangent(const Point<3> & p, int surfi1,
                              int surfi2,
                              const EdgePointGeomInfo & egi) const
    { throw Exception("Call GetTangent of " + Demangle(typeid(*this).name())); }

    virtual size_t GetEdgeIndex(const GeometryEdge& edge) const
    {
      for(auto i : Range(edges))
        if(edge.GetHash() == edges[i]->GetHash())
          return i;
      throw Exception("Couldn't find edge index");
    }
    virtual void Save (string filename) const;
    virtual void SaveToMeshFile (ostream & /* ost */) const { ; }
  };





  class DLL_HEADER GeometryRegister
  {
  public:
    virtual ~GeometryRegister();
    virtual NetgenGeometry * Load (string filename) const = 0;
    virtual NetgenGeometry * LoadFromMeshFile (istream & /* ist */) const { return NULL; }
    virtual class VisualScene * GetVisualScene (const NetgenGeometry * /* geom */) const
    { return NULL; }
    virtual void SetParameters (Tcl_Interp * /* interp */) { ; }
  };

  class DLL_HEADER GeometryRegisterArray : public NgArray<GeometryRegister*>
  {
  public:
    virtual ~GeometryRegisterArray()
    {
      for (int i = 0; i < Size(); i++)
        delete (*this)[i];
    }

    virtual shared_ptr<NetgenGeometry> LoadFromMeshFile (istream & ist) const;
  };

  // extern DLL_HEADER NgArray<GeometryRegister*> geometryregister; 
  extern DLL_HEADER GeometryRegisterArray geometryregister; 
}



#endif
