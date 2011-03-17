/*
  internal_water_filter.hpp

  (c) 2008 Tod D. Romo,
      Grossfield Lab,
      University of Rochester Medical and Dental School
*/


#if !defined(INTERNAL_WATER_FILTER_HPP)
#define INTERNAL_WATER_FILTER_HPP

#include <loos.hpp>
#include <sgrid.hpp>
#include <vector>


namespace WaterFilter {


  //! Base interface for water filter/picker
  class Base {
  public:
    Base() { }
    virtual ~Base() { }

    //! Given a molecule and a set of waters, pick which waters are inside
    /**
     * The result is a map of which waters are inside (1 = inside, 0 = not)
     */
    virtual std::vector<int> filter(const loos::AtomicGroup&, const loos::AtomicGroup&) =0;

    //! Calculate the appropriate bounding box (given the molecule)
    virtual std::vector<loos::GCoord> boundingBox(const loos::AtomicGroup&) =0;

    //! Calculate the volume of the region we can pick waters from...
    virtual double volume(void) =0;

    //! Just states the name of the filter/picker
    virtual std::string name(void) const =0;

  protected:
    std::vector<loos::GCoord> bdd_;
  };


  // --------------------------------------------------------------------------------

  //! Pick waters inside a bounding box
  /**
   * The bounding box is defined by the molecule.  Any water that lies
   * within that bounding box is then assumed to be internal.  The
   * bounding box size can be adjusted by a padding value.
   */
  class Box : public Base {
  public:
    Box(const double pad) : pad_(pad) { }
    virtual ~Box() { }

    virtual std::vector<int> filter(const loos::AtomicGroup&, const loos::AtomicGroup&);
    virtual std::vector<loos::GCoord> boundingBox(const loos::AtomicGroup&);

    virtual double volume(void);
    virtual std::string name(void) const;

  private:
    double pad_;
  };


  // --------------------------------------------------------------------------------

  //! Pick waters that are within a radius of the principal axis for a molecule
  /**
   * All atoms from the molecule are used to calculate the principal
   * axis.  The z-extent of the axis is determined by the z-values for
   * the bounding box of the molecule.  Any water that lies within
   * those z-values and is less than or equal to the radius given is
   * assumed to be internal.
   */

  class Axis : public Base {
  public:
    Axis(const double radius) : radius_(radius*radius) { }
    virtual ~Axis() { }

    virtual std::string name(void) const;
    virtual double volume(void);

    virtual std::vector<int> filter(const loos::AtomicGroup&, const loos::AtomicGroup&);
    std::vector<loos::GCoord> boundingBox(const loos::AtomicGroup&);

  private:
    loos::GCoord axis_, orig_;
    double radius_;
  };



  // --------------------------------------------------------------------------------

  //! Pick waters based on a grid-mask
  /**
   * Water coordinates are converted into grid coords.  If the
   * corresponding grid value is non-zero, then the water is deemed
   * internal.
   *
   * The bounding box is the bounding box for all non-zero grid elements.
   */
  class Blob : public Base {
  public:
    Blob(const lab::SGrid<int>& blob) : blob_(blob), bdd_set(false), vol(-1.0) { }
    virtual ~Blob() { }

    virtual std::string name(void) const;
    virtual double volume(void);

    std::vector<int> filter(const loos::AtomicGroup&, const loos::AtomicGroup&);
    std::vector<loos::GCoord> boundingBox(const loos::AtomicGroup&);

  private:
    lab::SGrid<int> blob_;
    bool bdd_set;
    double vol;
  };


  // --------------------------------------------------------------------------------

  //! Decorator base class for "decorating" the core water filters...
  class Decorator : public Base {
  public:
    Decorator(Base* p) : base(p) { }
    virtual ~Decorator() { }

    virtual std::string name(void) const { return(base->name()); }
    virtual double volume(void) { return(base->volume()); }
  
    virtual std::vector<int> filter(const loos::AtomicGroup& solv, const loos::AtomicGroup& prot) {
      return(base->filter(solv, prot));
    }

    virtual std::vector<loos::GCoord> boundingBox(const loos::AtomicGroup& prot) {
      return(base->boundingBox(prot));
    }

  private:
    Base *base;
  };


  //! Restrict waters to be within a given z-range
  class ZClipped : public Decorator {
  public:
    ZClipped(Base* p, const double zmin, const double zmax) : Decorator(p),
                                                                  zmin_(zmin), zmax_(zmax) { }
    virtual ~ZClipped() { }

    std::string name(void) const;
    std::vector<int> filter(const loos::AtomicGroup&, const loos::AtomicGroup&);
    std::vector<loos::GCoord> boundingBox(const loos::AtomicGroup&);

    double volume(void) { return(0.0); }

  private:
    double zmin_, zmax_;
  };



  //! Add bulk water back into the mask/map
  /**
   * When using a water filter, particularly with the ZClipped
   * decorator, you will end up with internal waters that don't
   * necessarily connect to bulk (for pore-like proteins).  You will
   * also not get bulk water layers if you're simulating a membrane
   * system.  To make it obvious that you've got water in there, use
   * the Bulked decorator.  This decorator examines waters not picked
   * by the internal filters.  If the water lies within the molecule's
   * bounding box (plus pad), and is higher or lower than the given
   * z-bounds, it is accepted as an "internal" water.  This will give
   * you a nice plane of bulk water over your protein/membrane.
   */
  class Bulked : public Decorator {
  public:
    Bulked(Base* p, const double pad, const double zmin, const double zmax) : Decorator(p),
                                                                                  pad_(pad),
                                                                                  zmin_(zmin), zmax_(zmax) { }
    virtual ~Bulked() { }

    std::string name(void) const;
    std::vector<int> filter(const loos::AtomicGroup&, const loos::AtomicGroup&);
    std::vector<loos::GCoord> boundingBox(const loos::AtomicGroup&);

    double volume(void) { return(0.0); }

  private:
    double pad_;
    double zmin_, zmax_;
  };

}


#endif
