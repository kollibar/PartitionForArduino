#ifndef __PARTITION_H__
#define __PARTITION_H__

#include <inttypes.h>
#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include <EEPROM_fct.h>
#include <Arduino.h>

#include "PartitionErreurs.h"


#define ATBEGIN  1
#define ATEND    2
#define AFTERPARTITION  3
#define ATADDR  4

const PROGMEM char TABLE_PARTITION_SIGNATURE[]={'B','M'};

#define NB_MAX_PARTITION  10

#define TABLE_PARTITION_TAILLE_ENTETE       16
#define TABLE_PARTITION_SIZE_SIGNATURE      3
#define TABLE_PARTITION_SIZE_CRC            2
#define TABLE_PARTITION_SIZE_NB_PARTITION   1
#define TABLE_PARTITION_SIZE_TRANSFERT_EN_COURS  1
#define TABLE_PARTITION_SIZE_RESERVED            9

#define TABLE_PARTITION_TAILLE_LIGNE_TABLE  20
#define TABLE_PARTITION_SIZE_NOM_PARTITION      8
#define TABLE_PARTITION_SIZE_VERSION_PARTITION  1
#define TABLE_PARTITION_SIZE_SIZE_PARTITION     2
#define TABLE_PARTITION_SIZE_ADDR_PARTITION     2
#define TABLE_PARTITION_SIZE_CRC_PARTITION      2
#define TABLE_PARTITION_SIZE_TAILLE_ACT_PARTITION     2
#define TABLE_PARTITION_SIZE_RESERVED_PARTITION 3

#define TABLE_PARTITION_POS_SIGNATURE  0
#define TABLE_PARTITION_POS_CRC             ( TABLE_PARTITION_POS_SIGNATURE + TABLE_PARTITION_SIZE_SIGNATURE )
#define TABLE_PARTITION_POS_NB_PARTITION    ( TABLE_PARTITION_POS_CRC + TABLE_PARTITION_SIZE_CRC )
#define TABLE_PARTITION_POS_TRANSFERT_EN_COURS  ( TABLE_PARTITION_POS_NB_PARTITION + TABLE_PARTITION_SIZE_NB_PARTITION )
#define TABLE_PARTITION_POS_LISTE_PARTITION ( TABLE_PARTITION_TAILLE_ENTETE )

#define TABLE_PARTITION_LIGNE_POS_NOM 0
#define TABLE_PARTITION_LIGNE_POS_VERSION ( TABLE_PARTITION_LIGNE_POS_NOM + TABLE_PARTITION_SIZE_NOM_PARTITION )
#define TABLE_PARTITION_LIGNE_POS_SIZE    ( TABLE_PARTITION_LIGNE_POS_VERSION + TABLE_PARTITION_SIZE_VERSION_PARTITION )
#define TABLE_PARTITION_LIGNE_POS_ADDR    ( TABLE_PARTITION_LIGNE_POS_SIZE + TABLE_PARTITION_SIZE_SIZE_PARTITION )
#define TABLE_PARTITION_LIGNE_POS_CRC     ( TABLE_PARTITION_LIGNE_POS_ADDR + TABLE_PARTITION_SIZE_ADDR_PARTITION )
#define TABLE_PARTITION_LIGNE_POS_TAILLE_ACT  ( TABLE_PARTITION_LIGNE_POS_CRC + TABLE_PARTITION_SIZE_TAILLE_ACT_PARTITION )
#define TABLE_PARTITION_LIGNE_POS_RESERVED    ( TABLE_PARTITION_LIGNE_POS_TAILLE_ACT + TABLE_PARTITION_SIZE_TAILLE_ACT_PARTITION )

#define TABLE_PARTITION_TAMPON_COPIE  10

#define TAILLE_EEPROM 4096

enum modePartition {
  modePartition_Lecture = 'l',
  modePartition_Ecriture = 'e',
  modePartition_LectureEcriture = 'a',
  modePartition_Erreur = 'r',
  modePartition_Erreur2 = 'R',
  modePartition_Close = 'c',
  modePartition_nonInit = 'n',
  modePartition_Init = 'i',
};

class Partition;

class TablePartition{
  friend class Partition;
public:
  uint8_t getNbPartition() const;
  uint8_t getNbMaxPartition() const;
  uint8_t getNbPatritionVide() const;

  uint8_t getIdPartition(const char* nom) const;

  size_t getNomPartition(uint8_t id, char* nom) const;
  uint8_t getVersionPartition(uint8_t id) const;
  size_t getTaillePartition(uint8_t id) const;
  size_t getAddrPartition(uint8_t id) const;

  size_t getTailleAct(uint8_t id) const;
  void setTailleAct(uint8_t id, size_t tailleAct);

  uint16_t getCRCPartition(uint8_t id) const;
  uint16_t calculCRCPartition(uint8_t id) const;
  void ecritCRCPartition(uint8_t id, uint16_t crc=0xFFFF);

  bool isPartitionned() const;
  void createTable(uint8_t nbPartition=4);

  uint8_t createPartition(char* nom, uint8_t tailleNom, size_t taille, uint8_t version, uint8_t position=ATBEGIN, uint16_t infoSup=0);

  bool getPartition(uint8_t id, Partition* p, modePartition mode=modePartition_Init);
  bool getPartition(const char* nom, Partition* p, modePartition mode=modePartition_Init);

  void printDebug(HardwareSerial* serial);

  size_t tailleMaxPartition();
protected:
  void writeSignature();
  bool readSignature() const;
  bool enteteValide() const;

  char* getAddressNomPartition(uint8_t id) const;
  char* getAddressLignePartition(uint8_t id) const;

  size_t sizeEnteteEtAnnuaire() const;
  void actualiseCRC();
  uint16_t litCRC() const;
  uint16_t calculCRC() const;

  bool movePartition(uint8_t id, size_t newAddr);
  bool resizePartition(uint8_t id, size_t newSize);
  bool createPartitionAt(char* nom, uint8_t tailleNom, size_t taille, uint8_t version, size_t addr);
  uint8_t chercheIdVide() const;
  void reordonneListePartition();
  size_t getEspaceLibreApresPartition(uint8_t id, bool ordonne = true) const;

  void setNomPartition(uint8_t id, char* nom, uint8_t tailleNom);
  void setTaillePartition(uint8_t id, size_t size);
  void setVersionPartition(uint8_t id, uint8_t version);
  void setAddrPartition(uint8_t id, size_t addr);

  bool addrDansPartition(size_t addr) const;

  uint8_t cherche1Partition(size_t addrDepart=0x0001, uint8_t nbMaxPartition=0xFF) const;

  void printDebugInfoG(HardwareSerial* serial);
  void printDebugPart(HardwareSerial* serial, uint8_t id);

  bool estZoneLibre(size_t addr, size_t taille, uint8_t partExclue=0xFF) const;
};

extern TablePartition TableDesPartitions;

class Partition{
  friend class TablePartition;
public:
  Partition();
  Partition(const char* nom, modePartition mode=modePartition_Init);
  ~Partition();

  size_t getTaille() const;

  size_t getTailleAct() const;
  bool setTailleAct(size_t tailleAct);

  modePartition getMode() const;
  bool setMode(modePartition mode);
  void close();

  bool setCurseur(size_t addr);
  size_t getCurseur() const;

  bool ecrit(const void* objet, size_t taille);
  template <typename T>
  bool ecrit(T& data);

  uint16_t ecrit(const void* objet, size_t taille, uint16_t position);
  template <typename T>
  bool ecrit(T& data, uint16_t position);

  uint16_t lit(void* objet, size_t taille, uint16_t position) const;
  template <typename T>
  T lit(uint16_t position) const;

  bool lit(void* objet, size_t taille);
  template <typename T>
  T lit();

  bool insertAt(uint16_t position, size_t taille, bool clear=false);
  bool deleteAt(uint16_t position, size_t taille);

  bool memset(uint16_t position, uint8_t valeur, size_t taille);

  bool controleCRC() const;
  uint16_t litCRC() const;
  uint16_t calculCRC() const;
  void actualiseCRC();

protected:
  void init(uint8_t id, modePartition mode);
  void setNull();
  size_t getAddr() const;

private:
  size_t taille;
  size_t addr;
  uint8_t id;

  modePartition mode;

  uint16_t curseur;
};

#endif
