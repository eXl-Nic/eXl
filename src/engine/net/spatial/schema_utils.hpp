// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include <core/base/coredef.hpp>
#include <math/vector3.hpp>

#include <improbable/c_schema.h>
#include <improbable/c_worker.h>


namespace SpatialGDK_C
{
  using WorkerAttributeSet = eXl::Vector<eXl::String>;
  using WorkerRequirementSet = eXl::Vector<WorkerAttributeSet>;
  using WriteAclMap = eXl::UnorderedMap<Worker_ComponentId, WorkerRequirementSet>;
  using StringToEntityMap = eXl::UnorderedMap<eXl::String, Worker_EntityId>;

  constexpr uint32_t s_EntityACLComponentId = 50;
  constexpr uint32_t s_MetaDataComponentId = 53;
  constexpr uint32_t s_PositionComponentId = 54;
  constexpr uint32_t s_InterestComponentId = 58;
  constexpr uint32_t s_WorkerComponentId = 60;
  constexpr uint32_t s_PlayerClientComponentId = 61;

  inline void AddStringToSchema(Schema_Object* Object, Schema_FieldId Id, const eXl::String& Value)
  {
	  uint8_t* StringBuffer = Schema_AllocateBuffer(Object, sizeof(char) * Value.length());
	  memcpy(StringBuffer, Value.c_str(), sizeof(char) * Value.length());
	  Schema_AddBytes(Object, Id, StringBuffer, sizeof(char) * Value.length());
  }

  inline eXl::String IndexStringFromSchema(const Schema_Object* Object, Schema_FieldId Id, uint32_t Index)
  {
	  uint32_t StringLength = Schema_IndexBytesLength(Object, Id, Index);
	  const uint8_t* Bytes = Schema_IndexBytes(Object, Id, Index);
	  return eXl::String(Bytes, Bytes + StringLength);
  }

  inline eXl::String GetStringFromSchema(const Schema_Object* Object, Schema_FieldId Id)
  {
	  return IndexStringFromSchema(Object, Id, 0);
  }

  inline bool GetBoolFromSchema(const Schema_Object* Object, Schema_FieldId Id)
  {
	  return !!Schema_GetBool(Object, Id);
  }

  inline void AddBytesToSchema(Schema_Object* Object, Schema_FieldId Id, const uint8_t* Data, uint32_t NumBytes)
  {
	  uint8_t* PayloadBuffer = Schema_AllocateBuffer(Object, sizeof(char) * NumBytes);
	  //FMemory::Memcpy(PayloadBuffer, Data, sizeof(char) * NumBytes);
	  memcpy(PayloadBuffer, Data, NumBytes);
	  Schema_AddBytes(Object, Id, PayloadBuffer, sizeof(char) * NumBytes);
  }

  //inline void AddBytesToSchema(Schema_Object* Object, Schema_FieldId Id, FBitWriter& Writer)
  //{
  //	AddBytesToSchema(Object, Id, Writer.GetData(), Writer.GetNumBytes());
  //}

  inline eXl::Vector<uint8_t> IndexBytesFromSchema(const Schema_Object* Object, Schema_FieldId Id, uint32_t Index)
  {
	  int32_t PayloadSize = Schema_IndexBytesLength(Object, Id, Index);
	  const uint8_t* Payload = Schema_IndexBytes(Object, Id, Index);
	  return eXl::Vector<uint8_t>(Payload, Payload + PayloadSize);
  }

  inline eXl::Vector<uint8_t> GetBytesFromSchema(const Schema_Object* Object, Schema_FieldId Id)
  {
	  return IndexBytesFromSchema(Object, Id, 0);
  }

  inline void AddWorkerRequirementSetToSchema(Schema_Object* Object, Schema_FieldId Id, const WorkerRequirementSet& Value)
  {
	  Schema_Object* RequirementSetObject = Schema_AddObject(Object, Id);
	  for (const WorkerAttributeSet& AttributeSet : Value)
	  {
		  Schema_Object* AttributeSetObject = Schema_AddObject(RequirementSetObject, 1);

		  for (const eXl::String& Attribute : AttributeSet)
		  {
			  AddStringToSchema(AttributeSetObject, 1, Attribute);
		  }
	  }
  }

  inline WorkerRequirementSet GetWorkerRequirementSetFromSchema(Schema_Object* Object, Schema_FieldId Id)
  {
	  Schema_Object* RequirementSetObject = Schema_GetObject(Object, Id);

	  int32_t AttributeSetCount = (int32_t)Schema_GetObjectCount(RequirementSetObject, 1);
	  WorkerRequirementSet RequirementSet;
	  RequirementSet.reserve(AttributeSetCount);

	  for (int32_t i = 0; i < AttributeSetCount; i++)
	  {
		  Schema_Object* AttributeSetObject = Schema_IndexObject(RequirementSetObject, 1, i);

		  int32_t AttributeCount = (int32_t)Schema_GetBytesCount(AttributeSetObject, 1);
		  WorkerAttributeSet AttributeSet;
		  AttributeSet.reserve(AttributeCount);

		  for (int32_t j = 0; j < AttributeCount; j++)
		  {
			  AttributeSet.push_back(IndexStringFromSchema(AttributeSetObject, 1, j));
		  }

		  RequirementSet.push_back(AttributeSet);
	  }

	  return RequirementSet;
  }

  #if 0
  inline void AddObjectRefToSchema(Schema_Object* Object, Schema_FieldId Id, const FUnrealObjectRef& ObjectRef)
  {
	  using namespace SpatialConstants;

	  Schema_Object* ObjectRefObject = Schema_AddObject(Object, Id);

	  Schema_AddEntityId(ObjectRefObject, UNREAL_OBJECT_REF_ENTITY_ID, ObjectRef.Entity);
	  Schema_AddUint32(ObjectRefObject, UNREAL_OBJECT_REF_OFFSET_ID, ObjectRef.Offset);
	  if (ObjectRef.Path)
	  {
		  AddStringToSchema(ObjectRefObject, UNREAL_OBJECT_REF_PATH_ID, *ObjectRef.Path);
		  Schema_AddBool(ObjectRefObject, UNREAL_OBJECT_REF_NO_LOAD_ON_CLIENT_ID, ObjectRef.bNoLoadOnClient);
	  }
	  if (ObjectRef.Outer)
	  {
		  AddObjectRefToSchema(ObjectRefObject, UNREAL_OBJECT_REF_OUTER_ID, *ObjectRef.Outer);
	  }
	  if (ObjectRef.bUseSingletonClassPath)
	  {
		  Schema_AddBool(ObjectRefObject, UNREAL_OBJECT_REF_USE_SINGLETON_CLASS_PATH_ID, ObjectRef.bUseSingletonClassPath);
	  }
  }

  FUnrealObjectRef GetObjectRefFromSchema(Schema_Object* Object, Schema_FieldId Id);

  inline FUnrealObjectRef IndexObjectRefFromSchema(Schema_Object* Object, Schema_FieldId Id, uint32 Index)
  {
	  using namespace SpatialConstants;

	  FUnrealObjectRef ObjectRef;

	  Schema_Object* ObjectRefObject = Schema_IndexObject(Object, Id, Index);

	  ObjectRef.Entity = Schema_GetEntityId(ObjectRefObject, UNREAL_OBJECT_REF_ENTITY_ID);
	  ObjectRef.Offset = Schema_GetUint32(ObjectRefObject, UNREAL_OBJECT_REF_OFFSET_ID);
	  if (Schema_GetObjectCount(ObjectRefObject, UNREAL_OBJECT_REF_PATH_ID) > 0)
	  {
		  ObjectRef.Path = GetStringFromSchema(ObjectRefObject, UNREAL_OBJECT_REF_PATH_ID);
	  }
	  if (Schema_GetBoolCount(ObjectRefObject, UNREAL_OBJECT_REF_NO_LOAD_ON_CLIENT_ID) > 0)
	  {
		  ObjectRef.bNoLoadOnClient = GetBoolFromSchema(ObjectRefObject, UNREAL_OBJECT_REF_NO_LOAD_ON_CLIENT_ID);
	  }
	  if (Schema_GetObjectCount(ObjectRefObject, UNREAL_OBJECT_REF_OUTER_ID) > 0)
	  {
		  ObjectRef.Outer = GetObjectRefFromSchema(ObjectRefObject, UNREAL_OBJECT_REF_OUTER_ID);
	  }
	  if (Schema_GetBoolCount(ObjectRefObject, UNREAL_OBJECT_REF_USE_SINGLETON_CLASS_PATH_ID) > 0)
	  {
		  ObjectRef.bUseSingletonClassPath = GetBoolFromSchema(ObjectRefObject, UNREAL_OBJECT_REF_USE_SINGLETON_CLASS_PATH_ID);
	  }

	  return ObjectRef;
  }

  inline FUnrealObjectRef GetObjectRefFromSchema(Schema_Object* Object, Schema_FieldId Id)
  {
	  return IndexObjectRefFromSchema(Object, Id, 0);
  }
  #endif

  inline void AddStringToEntityMapToSchema(Schema_Object* Object, Schema_FieldId Id, StringToEntityMap& Map)
  {
	  for (auto& Pair : Map)
	  {
		  Schema_Object* PairObject = Schema_AddObject(Object, Id);
		  AddStringToSchema(PairObject, SCHEMA_MAP_KEY_FIELD_ID, Pair.first);
		  Schema_AddEntityId(PairObject, SCHEMA_MAP_VALUE_FIELD_ID, Pair.second);
	  }
  }

  inline StringToEntityMap GetStringToEntityMapFromSchema(Schema_Object* Object, Schema_FieldId Id)
  {
	  StringToEntityMap Map;

	  int32_t MapCount = (int32_t)Schema_GetObjectCount(Object, Id);
	  for (int32_t i = 0; i < MapCount; i++)
	  {
		  Schema_Object* PairObject = Schema_IndexObject(Object, Id, i);

		  eXl::String String = GetStringFromSchema(PairObject, SCHEMA_MAP_KEY_FIELD_ID);
		  Worker_EntityId Entity = Schema_GetEntityId(PairObject, SCHEMA_MAP_VALUE_FIELD_ID);

		  Map.insert(std::make_pair(String, Entity));
	  }

	  return Map;
  }

  #if 0

  inline void AddRotatorToSchema(Schema_Object* Object, Schema_FieldId Id, FRotator Rotator)
  {
	  Schema_Object* RotatorObject = Schema_AddObject(Object, Id);

	  Schema_AddFloat(RotatorObject, 1, Rotator.Pitch);
	  Schema_AddFloat(RotatorObject, 2, Rotator.Yaw);
	  Schema_AddFloat(RotatorObject, 3, Rotator.Roll);
  }

  inline FRotator GetRotatorFromSchema(Schema_Object* Object, Schema_FieldId Id)
  {
	  FRotator Rotator;

	  Schema_Object* RotatorObject = Schema_GetObject(Object, Id);

	  Rotator.Pitch = Schema_GetFloat(RotatorObject, 1);
	  Rotator.Yaw = Schema_GetFloat(RotatorObject, 2);
	  Rotator.Roll = Schema_GetFloat(RotatorObject, 3);

	  return Rotator;
  }

  #endif

  struct EntityACL
  {
    WorkerRequirementSet m_ReadACL;
    eXl::UnorderedMap<uint32_t, WorkerRequirementSet> m_WriteACL;

    void ReadData(Schema_Object& iObject)
    {
      m_ReadACL.clear();
      m_WriteACL.clear();

      m_ReadACL = GetWorkerRequirementSetFromSchema(&iObject, 1);

      //Schema_Object* writeACLField = Schema_GetObject(&iObject, 2);
      int32_t numEntries = Schema_GetObjectCount(&iObject, 2);
      for (int32_t i = 0; i < numEntries; i++)
      {
        Schema_Object* PairObject = Schema_IndexObject(&iObject, 2, i);
        uint32_t compID = Schema_GetUint32(PairObject, 1);
        WorkerRequirementSet set = GetWorkerRequirementSetFromSchema(PairObject, 2);

        m_WriteACL.insert(std::make_pair(compID, std::move(set)));
      }
    }

    void WriteData(Schema_Object& iObject)
    {
      AddWorkerRequirementSetToSchema(&iObject, 1, m_ReadACL);

      for (auto const& entry : m_WriteACL)
      {
        Schema_Object* pair = Schema_AddObject(&iObject, 2);
        Schema_AddUint32(pair, 1, entry.first);
        AddWorkerRequirementSetToSchema(pair, 2, entry.second);
      }
    }

  };

  inline void AddVectorToSchema(Schema_Object* Object, Schema_FieldId Id, eXl::Vector3f Vector)
  {
    Schema_Object* VectorObject = Schema_AddObject(Object, Id);

    Schema_AddDouble(VectorObject, 1, Vector.X());
    Schema_AddDouble(VectorObject, 2, Vector.Y());
    Schema_AddDouble(VectorObject, 3, Vector.Z());
  }

  inline eXl::Vector3f GetVectorFromSchema(Schema_Object* Object, Schema_FieldId Id)
  {
    eXl::Vector3f Vector;

    Schema_Object* VectorObject = Schema_GetObject(Object, Id);

    Vector.X() = Schema_GetDouble(VectorObject, 1);
    Vector.Y() = Schema_GetDouble(VectorObject, 2);
    Vector.Z() = Schema_GetDouble(VectorObject, 3);

    return Vector;
  }

  using Coordinates = eXl::Vector3f;

  using EdgeLength = Coordinates;

  struct SphereConstraint
  {
    Coordinates Center;
    double Radius;
  };

  struct CylinderConstraint
  {
    Coordinates Center;
    double Radius;
  };

  struct BoxConstraint
  {
    Coordinates Center;
    EdgeLength EdgeLength;
  };

  struct RelativeSphereConstraint
  {
    double Radius;
  };

  struct RelativeCylinderConstraint
  {
    double Radius;
  };

  struct RelativeBoxConstraint
  {
    EdgeLength EdgeLength;
  };

  enum class ConstraintType
  {
    Invalid,
    Sphere,
    Cylinder,
    Box,
    RelativeSphere,
    RelativeCylinder,
    RelativeBox,
    EntityId,
    Component,
    And,
    Or
  };

  struct QueryConstraint
  {
    typedef eXl::Vector<QueryConstraint> ConstraintArray;
    union
    {
      SphereConstraint m_Sphere;
      CylinderConstraint m_Cylinder;
      BoxConstraint m_Box;
      RelativeSphereConstraint m_RelativeSphere;
      RelativeCylinderConstraint m_RelativeCylinder;
      RelativeBoxConstraint m_RelativeBox;
      int64_t m_EntityIdConstraint;
      uint32_t m_ComponentConstraint;
      ConstraintArray m_Constraints;
    };
    ConstraintType m_Type;

    static QueryConstraint CreateInvalid()
    {
      QueryConstraint newConstraint;
      return newConstraint;
    }

    static QueryConstraint CreateSphere(eXl::Vector3f const& iCenter, double iRadius)
    {
      QueryConstraint newConstraint;
      newConstraint.m_Type = ConstraintType::Sphere;
      newConstraint.m_Sphere.Center = iCenter;
      newConstraint.m_Sphere.Radius = iRadius;

      return newConstraint;
    }

    static QueryConstraint CreateCylinder(eXl::Vector3f const& iCenter, double iRadius)
    {
      QueryConstraint newConstraint;
      newConstraint.m_Type = ConstraintType::Cylinder;
      newConstraint.m_Cylinder.Center = iCenter;
      newConstraint.m_Cylinder.Radius = iRadius;

      return newConstraint;
    }

    static QueryConstraint CreateBox(eXl::Vector3f const& iCenter, eXl::Vector3f const& iDims)
    {
      QueryConstraint newConstraint;
      newConstraint.m_Type = ConstraintType::Box;
      newConstraint.m_Box.Center = iCenter;
      newConstraint.m_Box.EdgeLength = iDims;

      return newConstraint;
    }

    static QueryConstraint CreateRelativeSphere(double iRadius)
    {
      QueryConstraint newConstraint;
      newConstraint.m_Type = ConstraintType::RelativeSphere;
      newConstraint.m_RelativeSphere.Radius = iRadius;

      return newConstraint;
    }

    static QueryConstraint CreateRelativeCylinder(double iRadius)
    {
      QueryConstraint newConstraint;
      newConstraint.m_Type = ConstraintType::RelativeCylinder;
      newConstraint.m_RelativeCylinder.Radius = iRadius;

      return newConstraint;
    }

    static QueryConstraint CreateRelativeBox(eXl::Vector3f const& iDims)
    {
      QueryConstraint newConstraint;
      newConstraint.m_Type = ConstraintType::RelativeBox;
      newConstraint.m_RelativeBox.EdgeLength = iDims;

      return newConstraint;
    }

    static QueryConstraint CreateEntityId(Worker_EntityId iId)
    {
      QueryConstraint newConstraint;
      newConstraint.m_Type = ConstraintType::EntityId;
      newConstraint.m_EntityIdConstraint = iId;

      return newConstraint;
    }

    static QueryConstraint CreateComponent(Worker_ComponentId iId)
    {
      QueryConstraint newConstraint;
      newConstraint.m_Type = ConstraintType::Component;
      newConstraint.m_ComponentConstraint = iId;

      return newConstraint;
    }

    static QueryConstraint CreateOr(ConstraintArray&& iArray)
    {
      QueryConstraint newConstraint;
      newConstraint.m_Type = ConstraintType::Or;
      new (&newConstraint.m_Constraints) ConstraintArray(std::move(iArray));
      return newConstraint;
    }

    static QueryConstraint CreateAnd(ConstraintArray&& iArray)
    {
      QueryConstraint newConstraint;
      newConstraint.m_Type = ConstraintType::And;
      new (&newConstraint.m_Constraints) ConstraintArray(std::move(iArray));
      return newConstraint;
    }

    ~QueryConstraint()
    {
      Reset();
    }

    QueryConstraint(QueryConstraint const& iRHS)
    {
      Copy(iRHS);
    }

    QueryConstraint& operator=(QueryConstraint const& iRHS)
    {
      Copy(iRHS);
      return *this;
    }

    QueryConstraint& operator=(QueryConstraint&& iRHS)
    {
      Move(std::move(iRHS));
      return *this;
    }

    void Reset()
    {
      if (m_Type == ConstraintType::And || m_Type == ConstraintType::Or)
      {
        m_Constraints.~ConstraintArray();
      }
      m_Type = ConstraintType::Invalid;
    }

    void Copy(QueryConstraint const& iRHS)
    {
      Reset();
      m_Type = iRHS.m_Type;
      switch (m_Type)
      {
      case ConstraintType::Sphere:
        m_Sphere = iRHS.m_Sphere;
        break;
      case ConstraintType::Cylinder:
        m_Cylinder = iRHS.m_Cylinder;
        break;
      case ConstraintType::Box:
        m_Box = iRHS.m_Box;
        break;
      case ConstraintType::RelativeSphere:
        m_RelativeSphere = iRHS.m_RelativeSphere;
        break;
      case ConstraintType::RelativeCylinder:
        m_RelativeCylinder = iRHS.m_RelativeCylinder;
        break;
      case ConstraintType::RelativeBox:
        m_RelativeBox = iRHS.m_RelativeBox;
        break;
      case ConstraintType::EntityId:
        m_EntityIdConstraint = iRHS.m_EntityIdConstraint;
        break;
      case ConstraintType::Component:
        m_ComponentConstraint = iRHS.m_ComponentConstraint;
        break;
      case ConstraintType::And:
      case ConstraintType::Or:
        new (&m_Constraints) ConstraintArray();
        m_Constraints = iRHS.m_Constraints;
        break;
      }
    }

    void Move(QueryConstraint&& iRHS)
    {
      Reset();
      m_Type = iRHS.m_Type;
      switch (m_Type)
      {
      case ConstraintType::Sphere:
        m_Sphere = iRHS.m_Sphere;
        break;
      case ConstraintType::Cylinder:
        m_Cylinder = iRHS.m_Cylinder;
        break;
      case ConstraintType::Box:
        m_Box = iRHS.m_Box;
        break;
      case ConstraintType::RelativeSphere:
        m_RelativeSphere = iRHS.m_RelativeSphere;
        break;
      case ConstraintType::RelativeCylinder:
        m_RelativeCylinder = iRHS.m_RelativeCylinder;
        break;
      case ConstraintType::RelativeBox:
        m_RelativeBox = iRHS.m_RelativeBox;
        break;
      case ConstraintType::EntityId:
        m_EntityIdConstraint = iRHS.m_EntityIdConstraint;
        break;
      case ConstraintType::Component:
        m_ComponentConstraint = iRHS.m_ComponentConstraint;
        break;
      case ConstraintType::And:
      case ConstraintType::Or:
        new (&m_Constraints) ConstraintArray();
        m_Constraints = std::move(iRHS.m_Constraints);
        break;
      }
      iRHS.Reset();
    }

    private:
      QueryConstraint()
        : m_Type(ConstraintType::Invalid)
        , m_EntityIdConstraint(0)
      {
      }

  };

  struct Query
  {
    Query() : Constraint(QueryConstraint::CreateInvalid())
    {}

    QueryConstraint Constraint;

    // Either full_snapshot_result or a list of result_component_id should be provided. Providing both is invalid.
    bool m_FullSnapshotResult = true; // Whether all components should be included or none.
    eXl::Vector<uint32_t> m_ResultComponentId; // Which components should be included.

    // Used for frequency-based rate limiting. Represents the maximum frequency of updates for this
    // particular query. An empty option represents no rate-limiting (ie. updates are received
    // as soon as possible). Frequency is measured in Hz.
    //
    // If set, the time between consecutive updates will be at least 1/frequency. This is determined
    // at the time that updates are sent from the Runtime and may not necessarily correspond to the
    // time updates are received by the worker.
    //
    // If after an update has been sent, multiple updates are applied to a component, they will be
    // merged and sent as a single update after 1/frequency of the last sent update. When components
    // with events are merged, the resultant component will contain a concatenation of all the
    // events.
    //
    // If multiple queries match the same Entity-Component then the highest of all frequencies is
    // used.
    float m_Frequency = 0.0;
    bool m_UseFrequency = false;
  };

  struct ComponentInterest
  {
    eXl::Vector<Query> Queries;
  };

  inline void AddQueryConstraintToQuerySchema(Schema_Object* QueryObject, Schema_FieldId Id, const QueryConstraint& Constraint)
  {
    Schema_Object* QueryConstraintObject = Schema_AddObject(QueryObject, Id);

    
    switch (Constraint.m_Type)
    {
      //option<SphereConstraint> sphere_constraint = 1;
    case ConstraintType::Sphere:
    {
      Schema_Object* SphereConstraintObject = Schema_AddObject(QueryConstraintObject, 1);
      AddVectorToSchema(SphereConstraintObject, 1, Constraint.m_Sphere.Center);
      Schema_AddDouble(SphereConstraintObject, 2, Constraint.m_Sphere.Radius);

      break;
    }
    //option<CylinderConstraint> cylinder_constraint = 2;
    case ConstraintType::Cylinder:
    {
      Schema_Object* CylinderConstraintObject = Schema_AddObject(QueryConstraintObject, 2);
      AddVectorToSchema(CylinderConstraintObject, 1, Constraint.m_Cylinder.Center);
      Schema_AddDouble(CylinderConstraintObject, 2, Constraint.m_Cylinder.Radius);

      break;
    }

    //option<BoxConstraint> box_constraint = 3;
    case ConstraintType::Box:
    {
      Schema_Object* BoxConstraintObject = Schema_AddObject(QueryConstraintObject, 3);
      AddVectorToSchema(BoxConstraintObject, 1, Constraint.m_Box.Center);
      AddVectorToSchema(BoxConstraintObject, 2, Constraint.m_Box.EdgeLength);

      break;
    }

    //option<RelativeSphereConstraint> relative_sphere_constraint = 4;
    case ConstraintType::RelativeSphere:
    {
      Schema_Object* RelativeSphereConstraintObject = Schema_AddObject(QueryConstraintObject, 4);
      Schema_AddDouble(RelativeSphereConstraintObject, 1, Constraint.m_RelativeSphere.Radius);

      break;
    }

    //option<RelativeCylinderConstraint> relative_cylinder_constraint = 5;
    case ConstraintType::RelativeCylinder:
    {
      Schema_Object* RelativeCylinderConstraintObject = Schema_AddObject(QueryConstraintObject, 5);
      Schema_AddDouble(RelativeCylinderConstraintObject, 1, Constraint.m_RelativeCylinder.Radius);

      break;
    }

    //option<RelativeBoxConstraint> relative_box_constraint = 6;
    case ConstraintType::RelativeBox:
    {
      Schema_Object* RelativeBoxConstraintObject = Schema_AddObject(QueryConstraintObject, 6);
      AddVectorToSchema(RelativeBoxConstraintObject, 1, Constraint.m_RelativeBox.EdgeLength);

      break;
    }

    //option<int64> entity_id_constraint = 7;
    case ConstraintType::EntityId:
    {
      Schema_AddInt64(QueryConstraintObject, 7, Constraint.m_EntityIdConstraint);

      break;
    }

    //option<uint32> component_constraint = 8;
    case ConstraintType::Component:
    {
      Schema_AddUint32(QueryConstraintObject, 8, Constraint.m_ComponentConstraint);

      break;
    }

    //list<QueryConstraint> and_constraint = 9;
    case ConstraintType::And:
    {
      for (const QueryConstraint& AndConstraintEntry : Constraint.m_Constraints)
      {
        AddQueryConstraintToQuerySchema(QueryConstraintObject, 9, AndConstraintEntry);
      }

      break;
    }

    //list<QueryConstraint> or_constraint = 10;
    case ConstraintType::Or:
    {
      for (const QueryConstraint& OrConstraintEntry : Constraint.m_Constraints)
      {
        AddQueryConstraintToQuerySchema(QueryConstraintObject, 10, OrConstraintEntry);
      }

      break;
    }
    }
  }

  inline void AddQueryToComponentInterestSchema(Schema_Object* ComponentInterestObject, Schema_FieldId Id, const Query& Query)
  {
    //checkf(!(Query.FullSnapshotResult.IsSet() && Query.ResultComponentId.Num() > 0), TEXT("Either full_snapshot_result or a list of result_component_id should be provided. Providing both is invalid."));

    Schema_Object* QueryObject = Schema_AddObject(ComponentInterestObject, Id);

    AddQueryConstraintToQuerySchema(QueryObject, 1, Query.Constraint);

    if (Query.m_FullSnapshotResult)
    {
      Schema_AddBool(QueryObject, 2, Query.m_FullSnapshotResult);
    }
    if (!Query.m_FullSnapshotResult)
    {
      for (uint32_t ComponentId : Query.m_ResultComponentId)
      {
        Schema_AddUint32(QueryObject, 3, ComponentId);
      }
    }

    if (Query.m_UseFrequency)
    {
      Schema_AddFloat(QueryObject, 4, Query.m_Frequency);
    }
  }

  inline void AddComponentInterestToInterestSchema(Schema_Object* InterestObject, Schema_FieldId Id, const ComponentInterest& Value)
  {
    Schema_Object* ComponentInterestObject = Schema_AddObject(InterestObject, Id);

    for (const Query& QueryEntry : Value.Queries)
    {
      AddQueryToComponentInterestSchema(ComponentInterestObject, 1, QueryEntry);
    }
  }

  inline QueryConstraint IndexQueryConstraintFromSchema(Schema_Object* Object, Schema_FieldId Id, uint32_t Index)
  {
    Schema_Object* QueryConstraintObject = Schema_IndexObject(Object, Id, Index);

    // list<QueryConstraint> and_constraint = 9;
    const uint32_t AndConstraintCount = Schema_GetObjectCount(QueryConstraintObject, 9);
    // list<QueryConstraint> or_constraint = 10;
    const uint32_t OrConstraintCount = Schema_GetObjectCount(QueryConstraintObject, 10);

    // option<SphereConstraint> sphere_constraint = 1;
    if (Schema_GetObjectCount(QueryConstraintObject, 1) > 0)
    {
      Schema_Object* SphereConstraintObject = Schema_GetObject(QueryConstraintObject, 1);

      return QueryConstraint::CreateSphere(GetVectorFromSchema(SphereConstraintObject, 1), Schema_GetDouble(SphereConstraintObject, 2));
    }
    // option<CylinderConstraint> cylinder_constraint = 2;
    else if (Schema_GetObjectCount(QueryConstraintObject, 2) > 0)
    {
      Schema_Object* CylinderConstraintObject = Schema_GetObject(QueryConstraintObject, 2);

      return QueryConstraint::CreateCylinder(GetVectorFromSchema(CylinderConstraintObject, 1), Schema_GetDouble(CylinderConstraintObject, 2));
    }
    // option<BoxConstraint> box_constraint = 3;
    else if (Schema_GetObjectCount(QueryConstraintObject, 3) > 0)
    {
      Schema_Object* BoxConstraintObject = Schema_GetObject(QueryConstraintObject, 3);

      return QueryConstraint::CreateBox(GetVectorFromSchema(BoxConstraintObject, 1), GetVectorFromSchema(BoxConstraintObject, 2));
    }
    // option<RelativeSphereConstraint> relative_sphere_constraint = 4;
    else if (Schema_GetObjectCount(QueryConstraintObject, 4) > 0)
    {
      Schema_Object* RelativeSphereConstraintObject = Schema_GetObject(QueryConstraintObject, 4);

      return QueryConstraint::CreateRelativeSphere(Schema_GetDouble(RelativeSphereConstraintObject, 1));
    }
    // option<RelativeCylinderConstraint> relative_cylinder_constraint = 5;
    else if (Schema_GetObjectCount(QueryConstraintObject, 5) > 0)
    {
      Schema_Object* RelativeCylinderConstraintObject = Schema_GetObject(QueryConstraintObject, 5);

      return QueryConstraint::CreateRelativeCylinder(Schema_GetDouble(RelativeCylinderConstraintObject, 1));
    }
    // option<RelativeBoxConstraint> relative_box_constraint = 6;
    else if (Schema_GetObjectCount(QueryConstraintObject, 6) > 0)
    {
      Schema_Object* RelativeBoxConstraintObject = Schema_GetObject(QueryConstraintObject, 6);

      return QueryConstraint::CreateRelativeBox(GetVectorFromSchema(RelativeBoxConstraintObject, 1));
    }
    //option<int64> entity_id_constraint = 7;
    else if (Schema_GetObjectCount(QueryConstraintObject, 7) > 0)
    {
      Schema_Object* EntityIdConstraintObject = Schema_GetObject(QueryConstraintObject, 7);

      return QueryConstraint::CreateEntityId(Schema_GetInt64(EntityIdConstraintObject, 1));
    }
    // option<uint32> component_constraint = 8;
    else if (Schema_GetObjectCount(QueryConstraintObject, 8) > 0)
    {
      Schema_Object* ComponentConstraintObject = Schema_GetObject(QueryConstraintObject, 8);

      return QueryConstraint::CreateComponent(Schema_GetUint32(ComponentConstraintObject, 1));
    }
    else if (AndConstraintCount > 0)
    {
      QueryConstraint::ConstraintArray constraints;
      constraints.reserve(AndConstraintCount);

      for (uint32_t AndIndex = 0; AndIndex < AndConstraintCount; AndIndex++)
      {
        constraints.push_back(IndexQueryConstraintFromSchema(QueryConstraintObject, 9, AndIndex));
      }

      return QueryConstraint::CreateAnd(std::move(constraints));
    }
    else if (OrConstraintCount > 0)
    {
      QueryConstraint::ConstraintArray constraints;
      constraints.reserve(OrConstraintCount);

      for (uint32_t AndIndex = 0; AndIndex < AndConstraintCount; AndIndex++)
      {
        constraints.push_back(IndexQueryConstraintFromSchema(QueryConstraintObject, 9, AndIndex));
      }

      return QueryConstraint::CreateAnd(std::move(constraints));
    }

    return QueryConstraint::CreateInvalid();
  }

  inline QueryConstraint GetQueryConstraintFromSchema(Schema_Object* Object, Schema_FieldId Id)
  {
    return IndexQueryConstraintFromSchema(Object, Id, 1);
  }

  inline Query IndexQueryFromSchema(Schema_Object* Object, Schema_FieldId Id, uint32_t Index)
  {
    Query NewQuery;

    Schema_Object* QueryObject = Schema_IndexObject(Object, Id, Index);

    NewQuery.Constraint = GetQueryConstraintFromSchema(QueryObject, 1);

    if (Schema_GetObjectCount(QueryObject, 2) > 0)
    {
      NewQuery.m_FullSnapshotResult = GetBoolFromSchema(QueryObject, 2);
    }
    else
    {
      NewQuery.m_FullSnapshotResult = false;
    }

    uint32_t ResultComponentIdCount = Schema_GetObjectCount(QueryObject, 3);
    NewQuery.m_ResultComponentId.reserve(ResultComponentIdCount);
    for (uint32_t ComponentIdIndex = 0; ComponentIdIndex < ResultComponentIdCount; ComponentIdIndex++)
    {
      NewQuery.m_ResultComponentId.push_back(Schema_IndexUint32(QueryObject, 3, ComponentIdIndex));
    }

    if (Schema_GetObjectCount(QueryObject, 4) > 0)
    {
      NewQuery.m_UseFrequency = true;
      NewQuery.m_Frequency = Schema_GetFloat(QueryObject, 4);
    }
    else
    {
      NewQuery.m_UseFrequency = false;
    }

    return NewQuery;
  }

  inline ComponentInterest GetComponentInterestFromSchema(Schema_Object* Object, Schema_FieldId Id)
  {
    ComponentInterest NewComponentInterest;

    Schema_Object* ComponentInterestObject = Schema_GetObject(Object, Id);

    uint32_t QueryCount = Schema_GetObjectCount(ComponentInterestObject, 1);

    for (uint32_t QueryIndex = 0; QueryIndex < QueryCount; QueryIndex++)
    {
      NewComponentInterest.Queries.push_back(IndexQueryFromSchema(ComponentInterestObject, 1, QueryIndex));
    }

    return NewComponentInterest;
  }

  struct Interest
  {
    static const Worker_ComponentId ComponentId = s_InterestComponentId;

    Interest() = default;

    Interest(const Worker_ComponentData& Data)
    {
      Schema_Object* ComponentObject = Schema_GetComponentDataFields(Data.schema_type);

      uint32_t KVPairCount = Schema_GetObjectCount(ComponentObject, 1);
      for (uint32_t i = 0; i < KVPairCount; i++)
      {
        Schema_Object* KVPairObject = Schema_IndexObject(ComponentObject, 1, i);
        uint32_t Key = Schema_GetUint32(KVPairObject, SCHEMA_MAP_KEY_FIELD_ID);
        ComponentInterest Value = GetComponentInterestFromSchema(KVPairObject, SCHEMA_MAP_VALUE_FIELD_ID);

        ComponentInterestMap.insert(std::make_pair(Key, Value));
      }
    }

    bool IsEmpty()
    {
      return ComponentInterestMap.empty();
    }

    void ApplyComponentUpdate(const Worker_ComponentUpdate& Update)
    {
      Schema_Object* ComponentObject = Schema_GetComponentUpdateFields(Update.schema_type);

      // This is never emptied, so does not need an additional check for cleared fields
      uint32_t KVPairCount = Schema_GetObjectCount(ComponentObject, 1);
      if (KVPairCount > 0)
      {
        ComponentInterestMap.clear();
        for (uint32_t i = 0; i < KVPairCount; i++)
        {
          Schema_Object* KVPairObject = Schema_IndexObject(ComponentObject, 1, i);
          uint32_t Key = Schema_GetUint32(KVPairObject, SCHEMA_MAP_KEY_FIELD_ID);
          ComponentInterest Value = GetComponentInterestFromSchema(KVPairObject, SCHEMA_MAP_VALUE_FIELD_ID);

          ComponentInterestMap.insert(std::make_pair(Key, Value));
        }
      }
    }

    Worker_ComponentData CreateInterestData()
    {
      Worker_ComponentData Data = {};
      Data.component_id = ComponentId;
      Data.schema_type = Schema_CreateComponentData();
      Schema_Object* ComponentObject = Schema_GetComponentDataFields(Data.schema_type);

      FillComponentData(ComponentObject);

      return Data;
    }

    Worker_ComponentUpdate CreateInterestUpdate()
    {
      Worker_ComponentUpdate ComponentUpdate = {};
      ComponentUpdate.component_id = ComponentId;
      ComponentUpdate.schema_type = Schema_CreateComponentUpdate();
      Schema_Object* ComponentObject = Schema_GetComponentUpdateFields(ComponentUpdate.schema_type);

      FillComponentData(ComponentObject);

      return ComponentUpdate;
    }

    void FillComponentData(Schema_Object* InterestComponentObject)
    {
      for (const auto& KVPair : ComponentInterestMap)
      {
        Schema_Object* KVPairObject = Schema_AddObject(InterestComponentObject, 1);
        Schema_AddUint32(KVPairObject, SCHEMA_MAP_KEY_FIELD_ID, KVPair.first);
        AddComponentInterestToInterestSchema(KVPairObject, SCHEMA_MAP_VALUE_FIELD_ID, KVPair.second);
      }
    }

    eXl::UnorderedMap<uint32_t, ComponentInterest> ComponentInterestMap;
  };

} // namespace SpatialGDK
