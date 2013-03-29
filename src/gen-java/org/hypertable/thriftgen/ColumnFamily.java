/**
 * Autogenerated by Thrift Compiler (0.8.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
package org.hypertable.thriftgen;

import org.apache.thrift.scheme.IScheme;
import org.apache.thrift.scheme.SchemeFactory;
import org.apache.thrift.scheme.StandardScheme;

import org.apache.thrift.scheme.TupleScheme;
import org.apache.thrift.protocol.TTupleProtocol;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
import java.util.EnumMap;
import java.util.Set;
import java.util.HashSet;
import java.util.EnumSet;
import java.util.Collections;
import java.util.BitSet;
import java.nio.ByteBuffer;
import java.util.Arrays;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Describes a ColumnFamily
 * <dl>
 *   <dt>name</dt>
 *   <dd>Name of the column family</dd>
 * 
 *   <dt>ag</dt>
 *   <dd>Name of the access group for this CF</dd>
 * 
 *   <dt>max_versions</dt>
 *   <dd>Max versions of the same cell to be stored</dd>
 * 
 *   <dt>ttl</dt>
 *   <dd>Time to live for cells in the CF (ie delete cells older than this time)</dd>
 * </dl>
 */
public class ColumnFamily implements org.apache.thrift.TBase<ColumnFamily, ColumnFamily._Fields>, java.io.Serializable, Cloneable {
  private static final org.apache.thrift.protocol.TStruct STRUCT_DESC = new org.apache.thrift.protocol.TStruct("ColumnFamily");

  private static final org.apache.thrift.protocol.TField NAME_FIELD_DESC = new org.apache.thrift.protocol.TField("name", org.apache.thrift.protocol.TType.STRING, (short)1);
  private static final org.apache.thrift.protocol.TField AG_FIELD_DESC = new org.apache.thrift.protocol.TField("ag", org.apache.thrift.protocol.TType.STRING, (short)2);
  private static final org.apache.thrift.protocol.TField MAX_VERSIONS_FIELD_DESC = new org.apache.thrift.protocol.TField("max_versions", org.apache.thrift.protocol.TType.I32, (short)3);
  private static final org.apache.thrift.protocol.TField TTL_FIELD_DESC = new org.apache.thrift.protocol.TField("ttl", org.apache.thrift.protocol.TType.STRING, (short)4);

  private static final Map<Class<? extends IScheme>, SchemeFactory> schemes = new HashMap<Class<? extends IScheme>, SchemeFactory>();
  static {
    schemes.put(StandardScheme.class, new ColumnFamilyStandardSchemeFactory());
    schemes.put(TupleScheme.class, new ColumnFamilyTupleSchemeFactory());
  }

  public String name; // optional
  public String ag; // optional
  public int max_versions; // optional
  public String ttl; // optional

  /** The set of fields this struct contains, along with convenience methods for finding and manipulating them. */
  public enum _Fields implements org.apache.thrift.TFieldIdEnum {
    NAME((short)1, "name"),
    AG((short)2, "ag"),
    MAX_VERSIONS((short)3, "max_versions"),
    TTL((short)4, "ttl");

    private static final Map<String, _Fields> byName = new HashMap<String, _Fields>();

    static {
      for (_Fields field : EnumSet.allOf(_Fields.class)) {
        byName.put(field.getFieldName(), field);
      }
    }

    /**
     * Find the _Fields constant that matches fieldId, or null if its not found.
     */
    public static _Fields findByThriftId(int fieldId) {
      switch(fieldId) {
        case 1: // NAME
          return NAME;
        case 2: // AG
          return AG;
        case 3: // MAX_VERSIONS
          return MAX_VERSIONS;
        case 4: // TTL
          return TTL;
        default:
          return null;
      }
    }

    /**
     * Find the _Fields constant that matches fieldId, throwing an exception
     * if it is not found.
     */
    public static _Fields findByThriftIdOrThrow(int fieldId) {
      _Fields fields = findByThriftId(fieldId);
      if (fields == null) throw new IllegalArgumentException("Field " + fieldId + " doesn't exist!");
      return fields;
    }

    /**
     * Find the _Fields constant that matches name, or null if its not found.
     */
    public static _Fields findByName(String name) {
      return byName.get(name);
    }

    private final short _thriftId;
    private final String _fieldName;

    _Fields(short thriftId, String fieldName) {
      _thriftId = thriftId;
      _fieldName = fieldName;
    }

    public short getThriftFieldId() {
      return _thriftId;
    }

    public String getFieldName() {
      return _fieldName;
    }
  }

  // isset id assignments
  private static final int __MAX_VERSIONS_ISSET_ID = 0;
  private BitSet __isset_bit_vector = new BitSet(1);
  private _Fields optionals[] = {_Fields.NAME,_Fields.AG,_Fields.MAX_VERSIONS,_Fields.TTL};
  public static final Map<_Fields, org.apache.thrift.meta_data.FieldMetaData> metaDataMap;
  static {
    Map<_Fields, org.apache.thrift.meta_data.FieldMetaData> tmpMap = new EnumMap<_Fields, org.apache.thrift.meta_data.FieldMetaData>(_Fields.class);
    tmpMap.put(_Fields.NAME, new org.apache.thrift.meta_data.FieldMetaData("name", org.apache.thrift.TFieldRequirementType.OPTIONAL, 
        new org.apache.thrift.meta_data.FieldValueMetaData(org.apache.thrift.protocol.TType.STRING)));
    tmpMap.put(_Fields.AG, new org.apache.thrift.meta_data.FieldMetaData("ag", org.apache.thrift.TFieldRequirementType.OPTIONAL, 
        new org.apache.thrift.meta_data.FieldValueMetaData(org.apache.thrift.protocol.TType.STRING)));
    tmpMap.put(_Fields.MAX_VERSIONS, new org.apache.thrift.meta_data.FieldMetaData("max_versions", org.apache.thrift.TFieldRequirementType.OPTIONAL, 
        new org.apache.thrift.meta_data.FieldValueMetaData(org.apache.thrift.protocol.TType.I32)));
    tmpMap.put(_Fields.TTL, new org.apache.thrift.meta_data.FieldMetaData("ttl", org.apache.thrift.TFieldRequirementType.OPTIONAL, 
        new org.apache.thrift.meta_data.FieldValueMetaData(org.apache.thrift.protocol.TType.STRING)));
    metaDataMap = Collections.unmodifiableMap(tmpMap);
    org.apache.thrift.meta_data.FieldMetaData.addStructMetaDataMap(ColumnFamily.class, metaDataMap);
  }

  public ColumnFamily() {
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public ColumnFamily(ColumnFamily other) {
    __isset_bit_vector.clear();
    __isset_bit_vector.or(other.__isset_bit_vector);
    if (other.isSetName()) {
      this.name = other.name;
    }
    if (other.isSetAg()) {
      this.ag = other.ag;
    }
    this.max_versions = other.max_versions;
    if (other.isSetTtl()) {
      this.ttl = other.ttl;
    }
  }

  public ColumnFamily deepCopy() {
    return new ColumnFamily(this);
  }

  @Override
  public void clear() {
    this.name = null;
    this.ag = null;
    setMax_versionsIsSet(false);
    this.max_versions = 0;
    this.ttl = null;
  }

  public String getName() {
    return this.name;
  }

  public ColumnFamily setName(String name) {
    this.name = name;
    return this;
  }

  public void unsetName() {
    this.name = null;
  }

  /** Returns true if field name is set (has been assigned a value) and false otherwise */
  public boolean isSetName() {
    return this.name != null;
  }

  public void setNameIsSet(boolean value) {
    if (!value) {
      this.name = null;
    }
  }

  public String getAg() {
    return this.ag;
  }

  public ColumnFamily setAg(String ag) {
    this.ag = ag;
    return this;
  }

  public void unsetAg() {
    this.ag = null;
  }

  /** Returns true if field ag is set (has been assigned a value) and false otherwise */
  public boolean isSetAg() {
    return this.ag != null;
  }

  public void setAgIsSet(boolean value) {
    if (!value) {
      this.ag = null;
    }
  }

  public int getMax_versions() {
    return this.max_versions;
  }

  public ColumnFamily setMax_versions(int max_versions) {
    this.max_versions = max_versions;
    setMax_versionsIsSet(true);
    return this;
  }

  public void unsetMax_versions() {
    __isset_bit_vector.clear(__MAX_VERSIONS_ISSET_ID);
  }

  /** Returns true if field max_versions is set (has been assigned a value) and false otherwise */
  public boolean isSetMax_versions() {
    return __isset_bit_vector.get(__MAX_VERSIONS_ISSET_ID);
  }

  public void setMax_versionsIsSet(boolean value) {
    __isset_bit_vector.set(__MAX_VERSIONS_ISSET_ID, value);
  }

  public String getTtl() {
    return this.ttl;
  }

  public ColumnFamily setTtl(String ttl) {
    this.ttl = ttl;
    return this;
  }

  public void unsetTtl() {
    this.ttl = null;
  }

  /** Returns true if field ttl is set (has been assigned a value) and false otherwise */
  public boolean isSetTtl() {
    return this.ttl != null;
  }

  public void setTtlIsSet(boolean value) {
    if (!value) {
      this.ttl = null;
    }
  }

  public void setFieldValue(_Fields field, Object value) {
    switch (field) {
    case NAME:
      if (value == null) {
        unsetName();
      } else {
        setName((String)value);
      }
      break;

    case AG:
      if (value == null) {
        unsetAg();
      } else {
        setAg((String)value);
      }
      break;

    case MAX_VERSIONS:
      if (value == null) {
        unsetMax_versions();
      } else {
        setMax_versions((Integer)value);
      }
      break;

    case TTL:
      if (value == null) {
        unsetTtl();
      } else {
        setTtl((String)value);
      }
      break;

    }
  }

  public Object getFieldValue(_Fields field) {
    switch (field) {
    case NAME:
      return getName();

    case AG:
      return getAg();

    case MAX_VERSIONS:
      return Integer.valueOf(getMax_versions());

    case TTL:
      return getTtl();

    }
    throw new IllegalStateException();
  }

  /** Returns true if field corresponding to fieldID is set (has been assigned a value) and false otherwise */
  public boolean isSet(_Fields field) {
    if (field == null) {
      throw new IllegalArgumentException();
    }

    switch (field) {
    case NAME:
      return isSetName();
    case AG:
      return isSetAg();
    case MAX_VERSIONS:
      return isSetMax_versions();
    case TTL:
      return isSetTtl();
    }
    throw new IllegalStateException();
  }

  @Override
  public boolean equals(Object that) {
    if (that == null)
      return false;
    if (that instanceof ColumnFamily)
      return this.equals((ColumnFamily)that);
    return false;
  }

  public boolean equals(ColumnFamily that) {
    if (that == null)
      return false;

    boolean this_present_name = true && this.isSetName();
    boolean that_present_name = true && that.isSetName();
    if (this_present_name || that_present_name) {
      if (!(this_present_name && that_present_name))
        return false;
      if (!this.name.equals(that.name))
        return false;
    }

    boolean this_present_ag = true && this.isSetAg();
    boolean that_present_ag = true && that.isSetAg();
    if (this_present_ag || that_present_ag) {
      if (!(this_present_ag && that_present_ag))
        return false;
      if (!this.ag.equals(that.ag))
        return false;
    }

    boolean this_present_max_versions = true && this.isSetMax_versions();
    boolean that_present_max_versions = true && that.isSetMax_versions();
    if (this_present_max_versions || that_present_max_versions) {
      if (!(this_present_max_versions && that_present_max_versions))
        return false;
      if (this.max_versions != that.max_versions)
        return false;
    }

    boolean this_present_ttl = true && this.isSetTtl();
    boolean that_present_ttl = true && that.isSetTtl();
    if (this_present_ttl || that_present_ttl) {
      if (!(this_present_ttl && that_present_ttl))
        return false;
      if (!this.ttl.equals(that.ttl))
        return false;
    }

    return true;
  }

  @Override
  public int hashCode() {
    return 0;
  }

  public int compareTo(ColumnFamily other) {
    if (!getClass().equals(other.getClass())) {
      return getClass().getName().compareTo(other.getClass().getName());
    }

    int lastComparison = 0;
    ColumnFamily typedOther = (ColumnFamily)other;

    lastComparison = Boolean.valueOf(isSetName()).compareTo(typedOther.isSetName());
    if (lastComparison != 0) {
      return lastComparison;
    }
    if (isSetName()) {
      lastComparison = org.apache.thrift.TBaseHelper.compareTo(this.name, typedOther.name);
      if (lastComparison != 0) {
        return lastComparison;
      }
    }
    lastComparison = Boolean.valueOf(isSetAg()).compareTo(typedOther.isSetAg());
    if (lastComparison != 0) {
      return lastComparison;
    }
    if (isSetAg()) {
      lastComparison = org.apache.thrift.TBaseHelper.compareTo(this.ag, typedOther.ag);
      if (lastComparison != 0) {
        return lastComparison;
      }
    }
    lastComparison = Boolean.valueOf(isSetMax_versions()).compareTo(typedOther.isSetMax_versions());
    if (lastComparison != 0) {
      return lastComparison;
    }
    if (isSetMax_versions()) {
      lastComparison = org.apache.thrift.TBaseHelper.compareTo(this.max_versions, typedOther.max_versions);
      if (lastComparison != 0) {
        return lastComparison;
      }
    }
    lastComparison = Boolean.valueOf(isSetTtl()).compareTo(typedOther.isSetTtl());
    if (lastComparison != 0) {
      return lastComparison;
    }
    if (isSetTtl()) {
      lastComparison = org.apache.thrift.TBaseHelper.compareTo(this.ttl, typedOther.ttl);
      if (lastComparison != 0) {
        return lastComparison;
      }
    }
    return 0;
  }

  public _Fields fieldForId(int fieldId) {
    return _Fields.findByThriftId(fieldId);
  }

  public void read(org.apache.thrift.protocol.TProtocol iprot) throws org.apache.thrift.TException {
    schemes.get(iprot.getScheme()).getScheme().read(iprot, this);
  }

  public void write(org.apache.thrift.protocol.TProtocol oprot) throws org.apache.thrift.TException {
    schemes.get(oprot.getScheme()).getScheme().write(oprot, this);
  }

  @Override
  public String toString() {
    StringBuilder sb = new StringBuilder("ColumnFamily(");
    boolean first = true;

    if (isSetName()) {
      sb.append("name:");
      if (this.name == null) {
        sb.append("null");
      } else {
        sb.append(this.name);
      }
      first = false;
    }
    if (isSetAg()) {
      if (!first) sb.append(", ");
      sb.append("ag:");
      if (this.ag == null) {
        sb.append("null");
      } else {
        sb.append(this.ag);
      }
      first = false;
    }
    if (isSetMax_versions()) {
      if (!first) sb.append(", ");
      sb.append("max_versions:");
      sb.append(this.max_versions);
      first = false;
    }
    if (isSetTtl()) {
      if (!first) sb.append(", ");
      sb.append("ttl:");
      if (this.ttl == null) {
        sb.append("null");
      } else {
        sb.append(this.ttl);
      }
      first = false;
    }
    sb.append(")");
    return sb.toString();
  }

  public void validate() throws org.apache.thrift.TException {
    // check for required fields
  }

  private void writeObject(java.io.ObjectOutputStream out) throws java.io.IOException {
    try {
      write(new org.apache.thrift.protocol.TCompactProtocol(new org.apache.thrift.transport.TIOStreamTransport(out)));
    } catch (org.apache.thrift.TException te) {
      throw new java.io.IOException(te);
    }
  }

  private void readObject(java.io.ObjectInputStream in) throws java.io.IOException, ClassNotFoundException {
    try {
      // it doesn't seem like you should have to do this, but java serialization is wacky, and doesn't call the default constructor.
      __isset_bit_vector = new BitSet(1);
      read(new org.apache.thrift.protocol.TCompactProtocol(new org.apache.thrift.transport.TIOStreamTransport(in)));
    } catch (org.apache.thrift.TException te) {
      throw new java.io.IOException(te);
    }
  }

  private static class ColumnFamilyStandardSchemeFactory implements SchemeFactory {
    public ColumnFamilyStandardScheme getScheme() {
      return new ColumnFamilyStandardScheme();
    }
  }

  private static class ColumnFamilyStandardScheme extends StandardScheme<ColumnFamily> {

    public void read(org.apache.thrift.protocol.TProtocol iprot, ColumnFamily struct) throws org.apache.thrift.TException {
      org.apache.thrift.protocol.TField schemeField;
      iprot.readStructBegin();
      while (true)
      {
        schemeField = iprot.readFieldBegin();
        if (schemeField.type == org.apache.thrift.protocol.TType.STOP) { 
          break;
        }
        switch (schemeField.id) {
          case 1: // NAME
            if (schemeField.type == org.apache.thrift.protocol.TType.STRING) {
              struct.name = iprot.readString();
              struct.setNameIsSet(true);
            } else { 
              org.apache.thrift.protocol.TProtocolUtil.skip(iprot, schemeField.type);
            }
            break;
          case 2: // AG
            if (schemeField.type == org.apache.thrift.protocol.TType.STRING) {
              struct.ag = iprot.readString();
              struct.setAgIsSet(true);
            } else { 
              org.apache.thrift.protocol.TProtocolUtil.skip(iprot, schemeField.type);
            }
            break;
          case 3: // MAX_VERSIONS
            if (schemeField.type == org.apache.thrift.protocol.TType.I32) {
              struct.max_versions = iprot.readI32();
              struct.setMax_versionsIsSet(true);
            } else { 
              org.apache.thrift.protocol.TProtocolUtil.skip(iprot, schemeField.type);
            }
            break;
          case 4: // TTL
            if (schemeField.type == org.apache.thrift.protocol.TType.STRING) {
              struct.ttl = iprot.readString();
              struct.setTtlIsSet(true);
            } else { 
              org.apache.thrift.protocol.TProtocolUtil.skip(iprot, schemeField.type);
            }
            break;
          default:
            org.apache.thrift.protocol.TProtocolUtil.skip(iprot, schemeField.type);
        }
        iprot.readFieldEnd();
      }
      iprot.readStructEnd();

      // check for required fields of primitive type, which can't be checked in the validate method
      struct.validate();
    }

    public void write(org.apache.thrift.protocol.TProtocol oprot, ColumnFamily struct) throws org.apache.thrift.TException {
      struct.validate();

      oprot.writeStructBegin(STRUCT_DESC);
      if (struct.name != null) {
        if (struct.isSetName()) {
          oprot.writeFieldBegin(NAME_FIELD_DESC);
          oprot.writeString(struct.name);
          oprot.writeFieldEnd();
        }
      }
      if (struct.ag != null) {
        if (struct.isSetAg()) {
          oprot.writeFieldBegin(AG_FIELD_DESC);
          oprot.writeString(struct.ag);
          oprot.writeFieldEnd();
        }
      }
      if (struct.isSetMax_versions()) {
        oprot.writeFieldBegin(MAX_VERSIONS_FIELD_DESC);
        oprot.writeI32(struct.max_versions);
        oprot.writeFieldEnd();
      }
      if (struct.ttl != null) {
        if (struct.isSetTtl()) {
          oprot.writeFieldBegin(TTL_FIELD_DESC);
          oprot.writeString(struct.ttl);
          oprot.writeFieldEnd();
        }
      }
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }

  }

  private static class ColumnFamilyTupleSchemeFactory implements SchemeFactory {
    public ColumnFamilyTupleScheme getScheme() {
      return new ColumnFamilyTupleScheme();
    }
  }

  private static class ColumnFamilyTupleScheme extends TupleScheme<ColumnFamily> {

    @Override
    public void write(org.apache.thrift.protocol.TProtocol prot, ColumnFamily struct) throws org.apache.thrift.TException {
      TTupleProtocol oprot = (TTupleProtocol) prot;
      BitSet optionals = new BitSet();
      if (struct.isSetName()) {
        optionals.set(0);
      }
      if (struct.isSetAg()) {
        optionals.set(1);
      }
      if (struct.isSetMax_versions()) {
        optionals.set(2);
      }
      if (struct.isSetTtl()) {
        optionals.set(3);
      }
      oprot.writeBitSet(optionals, 4);
      if (struct.isSetName()) {
        oprot.writeString(struct.name);
      }
      if (struct.isSetAg()) {
        oprot.writeString(struct.ag);
      }
      if (struct.isSetMax_versions()) {
        oprot.writeI32(struct.max_versions);
      }
      if (struct.isSetTtl()) {
        oprot.writeString(struct.ttl);
      }
    }

    @Override
    public void read(org.apache.thrift.protocol.TProtocol prot, ColumnFamily struct) throws org.apache.thrift.TException {
      TTupleProtocol iprot = (TTupleProtocol) prot;
      BitSet incoming = iprot.readBitSet(4);
      if (incoming.get(0)) {
        struct.name = iprot.readString();
        struct.setNameIsSet(true);
      }
      if (incoming.get(1)) {
        struct.ag = iprot.readString();
        struct.setAgIsSet(true);
      }
      if (incoming.get(2)) {
        struct.max_versions = iprot.readI32();
        struct.setMax_versionsIsSet(true);
      }
      if (incoming.get(3)) {
        struct.ttl = iprot.readString();
        struct.setTtlIsSet(true);
      }
    }
  }

}
