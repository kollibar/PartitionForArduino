#include "Partition.h"


TablePartition TableDesPartitions=TablePartition();

bool TablePartition::getPartition(uint8_t id, Partition* p, modePartition mode){
  if( this->getTaillePartition(id) == 0 ) return false;

  p->init(id, mode);

  return true;
}

bool TablePartition::getPartition(const char* nom, Partition* p, modePartition mode){
  uint8_t id=this->getIdPartition(nom);
  if( id == 0xFF) return false;

  return this->getPartition(id, p, mode);
}


bool TablePartition::readSignature() const{
  char tampon[3];
  eeprom_read_block(tampon, (const void *)TABLE_PARTITION_POS_SIGNATURE, 3);
  return (strcmp_P(tampon, TABLE_PARTITION_SIGNATURE)==0);
}
void TablePartition::writeSignature(){
  char tampon[3];
  memcpy_P(tampon, TABLE_PARTITION_SIGNATURE, 3);
  eeprom_write_block(tampon, (void *)TABLE_PARTITION_POS_SIGNATURE, 3);
}

bool TablePartition::enteteValide() const{
  if( ! this->readSignature() ) return false;
  uint8_t nb=this->getNbMaxPartition();

  if( nb > NB_MAX_PARTITION ) return false;

  if( this->litCRC() != this->calculCRC() ) return false;

  return true;
}

bool TablePartition::isPartitionned() const{
  return this->enteteValide();
}

void TablePartition::actualiseCRC(){
  uint16_t crc=crc16Memoire(TABLE_PARTITION_POS_NB_PARTITION, this->sizeEnteteEtAnnuaire() - TABLE_PARTITION_POS_NB_PARTITION);
  eeprom_update_word((uint16_t *)TABLE_PARTITION_POS_CRC, crc);
}

uint16_t TablePartition::litCRC() const{
  return eeprom_read_word((const uint16_t *)TABLE_PARTITION_POS_CRC);
}

uint16_t TablePartition::calculCRC() const{
  size_t tailleEntete=this->sizeEnteteEtAnnuaire();

  return crc16Memoire(TABLE_PARTITION_POS_NB_PARTITION, tailleEntete - TABLE_PARTITION_POS_NB_PARTITION);
}

size_t TablePartition::sizeEnteteEtAnnuaire() const{
  return this->getNbMaxPartition() * TABLE_PARTITION_TAILLE_LIGNE_TABLE + TABLE_PARTITION_TAILLE_ENTETE;
}

uint8_t TablePartition::getNbMaxPartition() const{
  return eeprom_read_byte((const uint8_t *)TABLE_PARTITION_POS_NB_PARTITION);
}

uint8_t TablePartition::getNbPatritionVide() const{
  return this->getNbMaxPartition() - this->getNbPartition();
}

uint8_t TablePartition::getNbPartition() const{
  uint8_t nb=0;
  for(uint8_t i=0; i < this->getNbMaxPartition(); i++){
    if( this->getTaillePartition(i) != 0) nb++;
  }
  return nb;
}

size_t TablePartition::getTaillePartition(uint8_t id) const{
  return eeprom_read_word((const uint16_t *)TABLE_PARTITION_TAILLE_ENTETE + id * TABLE_PARTITION_TAILLE_LIGNE_TABLE + TABLE_PARTITION_LIGNE_POS_SIZE);
}

size_t TablePartition::getAddrPartition(uint8_t id) const{
  return eeprom_read_word((const uint16_t *)TABLE_PARTITION_TAILLE_ENTETE + id * TABLE_PARTITION_TAILLE_LIGNE_TABLE + TABLE_PARTITION_LIGNE_POS_ADDR);
}

uint8_t TablePartition::getVersionPartition(uint8_t id) const{
  return eeprom_read_byte((const uint8_t *)TABLE_PARTITION_TAILLE_ENTETE + id * TABLE_PARTITION_TAILLE_LIGNE_TABLE + TABLE_PARTITION_LIGNE_POS_VERSION);
}

size_t TablePartition::getTailleAct(uint8_t id) const{
  return eeprom_read_word((const uint16_t *)TABLE_PARTITION_TAILLE_ENTETE + id * TABLE_PARTITION_TAILLE_LIGNE_TABLE + TABLE_PARTITION_LIGNE_POS_TAILLE_ACT);
}
void TablePartition::setTailleAct(uint8_t id, size_t tailleAct){
  eeprom_update_word((uint16_t *)TABLE_PARTITION_TAILLE_ENTETE + id * TABLE_PARTITION_TAILLE_LIGNE_TABLE + TABLE_PARTITION_LIGNE_POS_TAILLE_ACT, tailleAct);
}

uint8_t TablePartition::getIdPartition(const char* nom) const{
  for(uint8_t id=0; id < this->getNbMaxPartition(); id++){
    if( strcmp_E(nom, this->getAddressNomPartition(id), TABLE_PARTITION_SIZE_NOM_PARTITION) == 0 && this->getTaillePartition(id) != 0 ) return id;
  }
  // non trouvé
  return 0xFF;
}

char* TablePartition::getAddressNomPartition(uint8_t id) const{
  return (char*)( TABLE_PARTITION_TAILLE_ENTETE + id * TABLE_PARTITION_TAILLE_LIGNE_TABLE + TABLE_PARTITION_LIGNE_POS_NOM );
}

char* TablePartition::getAddressLignePartition(uint8_t id) const{
  return (char*)( TABLE_PARTITION_TAILLE_ENTETE + id * TABLE_PARTITION_TAILLE_LIGNE_TABLE );
}

bool TablePartition::movePartition(uint8_t id, size_t newAddr){
  if( ! this->estZoneLibre(newAddr, this->getTaillePartition(id), id)) return false;

  char tampon[TABLE_PARTITION_TAMPON_COPIE];
  size_t resteAcopier = this->getTaillePartition(id);

  if( this->getAddrPartition(id) < newAddr){ // avance la partition

    size_t pos1=this->getAddrPartition(id)+this->getTaillePartition(id);
    size_t pos2=newAddr + this->getTaillePartition(id);
    while( resteAcopier > 0){
      uint8_t s=min(TABLE_PARTITION_TAMPON_COPIE, resteAcopier);

      eeprom_read_block(tampon, (void*)(pos1-s), s);
      eeprom_update_block(tampon, (void*)(pos2-s), s);

      resteAcopier -= s;
    }
  } else { // recule la partition
    size_t pos1=this->getAddrPartition(id);
    size_t pos2=newAddr;
    while( resteAcopier > 0){
      uint8_t s=min(TABLE_PARTITION_TAMPON_COPIE, resteAcopier);

      eeprom_read_block(tampon, (void*)pos1, s);
      eeprom_update_block(tampon, (void*)pos2, s);

      resteAcopier += s;
    }
  }
  return true;
}
bool TablePartition::resizePartition(uint8_t id, size_t newSize){
  if( this->getTaillePartition(id) < newSize ){ // agrandissement !
    if( ! this->estZoneLibre(this->getAddrPartition(id), newSize, id)) return false;
  }

  this->setTaillePartition(id, newSize);

  return true;
}

size_t TablePartition::tailleMaxPartition(){
  return ( TAILLE_EEPROM - ( TABLE_PARTITION_TAILLE_ENTETE + this->getNbMaxPartition() * TABLE_PARTITION_TAILLE_LIGNE_TABLE ));
}

uint8_t TablePartition::createPartition(char* nom, uint8_t tailleNom, size_t taille, uint8_t version, uint8_t position, uint16_t infoSup){
  if( taille > this->tailleMaxPartition() ) return TABLEPART_ERREUR_TAILLE_SUPPERIEUR_TAILLE_MAX_PARTITION; // taille Max partition

  if( this->getNbPatritionVide() == 0 ) return TABLEPART_ERREUR_PLUS_DE_PARTITION_DISPONIBNLE;

  this->reordonneListePartition();

  if( position==ATBEGIN ){
    if( this->getTaillePartition(0) == 0){  // aucune partition
      this->createPartitionAt(nom, tailleNom, taille, version, ( TABLE_PARTITION_TAILLE_ENTETE + this->getNbMaxPartition() * TABLE_PARTITION_TAILLE_LIGNE_TABLE ) );
    }
    for(uint8_t id=0;id<this->getNbMaxPartition();id++){

      if( this->getEspaceLibreApresPartition(id, true) > taille ){ // espace libre suffisant !
        if( this->createPartitionAt(nom, tailleNom, taille, version, this->getAddrPartition(id)+this->getTaillePartition(id)) ) return TABLEPART_ERREUR_OK;
        else return TABLEPART_ERREUR_ESPACE_INSUFFISANT_OU_FRAGMENTE;
      }
    }
    return TABLEPART_ERREUR_ESPACE_INSUFFISANT_OU_FRAGMENTE;
  } else if(position == ATEND ){
    if( this->getTaillePartition(0) == 0){  // aucune partition
      this->createPartitionAt(nom, tailleNom, taille, version, TAILLE_EEPROM - taille );
    }
    uint8_t dernierePart=0;
    for(uint8_t id=0;id<this->getNbMaxPartition();id++){
      if( this->getTaillePartition(id) == 0 ) dernierePart = id;
    }
    return this->createPartition(nom, tailleNom, taille, version, AFTERPARTITION, dernierePart);

  } else if(position == AFTERPARTITION ){
    uint8_t id=(uint8_t)infoSup;
    if( this->getEspaceLibreApresPartition(infoSup,true) > taille ){
      if( this->createPartitionAt(nom, tailleNom, taille, version, this->getAddrPartition(id)+this->getTaillePartition(id) ) ) return TABLEPART_ERREUR_OK;
      else return TABLEPART_ERREUR_ESPACE_INSUFFISANT_OU_FRAGMENTE;
    }
    return TABLEPART_ERREUR_ESPACE_INSUFFISANT_APRES_PARTITION;
  } else if( position == ATADDR){

    if( this->createPartitionAt(nom, tailleNom, taille, version, infoSup)) return TABLEPART_ERREUR_OK;
    else return TABLEPART_ERREUR_ESPACE_INSUFFISANT_OU_FRAGMENTE;

  }
  return TABLEPART_ERREUR_ERREUR_ARGUMENT; // oosition inconnu
}

bool TablePartition::estZoneLibre(size_t addr, size_t taille, uint8_t partExclue) const{
  if( addr > TAILLE_EEPROM || taille > TAILLE_EEPROM || addr + taille > TAILLE_EEPROM ) return false;

  if( addr < this->sizeEnteteEtAnnuaire() ) return false;

  for(uint8_t id=0;id<this->getNbPartition();id++){
    if( id == partExclue ) continue;

    if( addr >= this->getAddrPartition(id) && addr < ( this->getAddrPartition(id) + this->getTaillePartition(id) ) ) return false;
    if( ( addr+taille ) > this->getAddrPartition(id) && ( addr+taille ) <= ( this->getAddrPartition(id) + this->getTaillePartition(id) ) ) return false;
    if( addr < this->getAddrPartition(id) && ( addr+taille ) >= ( this->getAddrPartition(id) + this->getTaillePartition(id) ) ) return false;
  }
  return true;
}

size_t TablePartition::getEspaceLibreApresPartition(uint8_t id, bool ordonne) const{
  if(ordonne){
    return ( ( ( id == this->getNbMaxPartition()-1) || ( this->getTaillePartition(id+1)==0 ) )?TAILLE_EEPROM:this->getAddrPartition(id+1) )-(this->getAddrPartition(id)+this->getTaillePartition(id));
  } else {
    size_t finPart=this->getAddrPartition(id)+this->getTaillePartition(id);
    size_t min=TAILLE_EEPROM;
    for(uint8_t i=0;i<this->getNbMaxPartition();i++){
      if( this->getAddrPartition(i) >= finPart && this->getAddrPartition(i) < min ) min=this->getAddrPartition(i);
    }
    return( min - finPart );
  }
}

bool TablePartition::createPartitionAt(char* nom, uint8_t tailleNom, size_t taille, uint8_t version, size_t addr){
  if( ! this->estZoneLibre(addr, taille) ) return false;

  uint8_t id=this->chercheIdVide();
  if( id == 0xFF ) return false;

  this->setNomPartition(id, nom, tailleNom);
  this->setTaillePartition(id, taille);
  this->setVersionPartition(id, version);
  this->setAddrPartition(id, addr);
  return true;
}

uint8_t TablePartition::chercheIdVide() const{
  for(uint8_t id=0; id < this->getNbMaxPartition(); id++){
    if( this->getTaillePartition(id) == 0 ) return id;
  }
  return 0xFF;
}

void TablePartition::setNomPartition(uint8_t id, char* nom, uint8_t tailleNom){
  eeprom_update_block(nom, this->getAddressNomPartition(id), min(TABLE_PARTITION_SIZE_NOM_PARTITION, tailleNom));
}
void TablePartition::setTaillePartition(uint8_t id, size_t size){
  eeprom_update_word((uint16_t *)( TABLE_PARTITION_TAILLE_ENTETE + id * TABLE_PARTITION_TAILLE_LIGNE_TABLE + TABLE_PARTITION_LIGNE_POS_SIZE ), size);
}
void TablePartition::setVersionPartition(uint8_t id, uint8_t version){
  eeprom_update_byte((uint8_t *)( TABLE_PARTITION_TAILLE_ENTETE + id * TABLE_PARTITION_TAILLE_LIGNE_TABLE + TABLE_PARTITION_LIGNE_POS_VERSION ), version);
}
void TablePartition::setAddrPartition(uint8_t id, size_t addr){
  eeprom_update_word((uint16_t *)( TABLE_PARTITION_TAILLE_ENTETE + id * TABLE_PARTITION_TAILLE_LIGNE_TABLE + TABLE_PARTITION_LIGNE_POS_ADDR ), addr);
}

void TablePartition::reordonneListePartition(){
  uint8_t nbMaxPartition=this->getNbMaxPartition();

  size_t addrPart[NB_MAX_PARTITION];
  for(uint8_t id=0;id<nbMaxPartition;id++){
    addrPart[id]=this->getAddrPartition(id);
    if( this->getTaillePartition(id) == 0 ) addrPart[id]=0;
  }

  size_t addrDepart=0x0001;

  for(uint8_t id=0;id<nbMaxPartition;id++){
    uint8_t id1=0xFF;
    for(uint8_t idx=id; idx<nbMaxPartition; idx++){
      if( addrPart[idx] > addrDepart && ( id1 == 0xFF || addrPart[id1] > addrPart[idx] ) ){
        id1=idx;
      }
    }

    // inversion de partition id et partition idx
    for(uint8_t i=0;i<TABLE_PARTITION_TAILLE_LIGNE_TABLE;i+=TABLE_PARTITION_TAMPON_COPIE/2){
      char tampon[TABLE_PARTITION_TAMPON_COPIE/2];
      char tampon2[TABLE_PARTITION_TAMPON_COPIE/2];

      eeprom_read_block(tampon, this->getAddressLignePartition(id), min(TABLE_PARTITION_TAMPON_COPIE/2, TABLE_PARTITION_TAILLE_LIGNE_TABLE-i));
      eeprom_read_block(tampon2, this->getAddressLignePartition(id1), min(TABLE_PARTITION_TAMPON_COPIE/2, TABLE_PARTITION_TAILLE_LIGNE_TABLE-i));

      eeprom_update_block(tampon2, this->getAddressLignePartition(id), min(TABLE_PARTITION_TAMPON_COPIE/2, TABLE_PARTITION_TAILLE_LIGNE_TABLE-i));
      eeprom_update_block(tampon, this->getAddressLignePartition(id1), min(TABLE_PARTITION_TAMPON_COPIE/2, TABLE_PARTITION_TAILLE_LIGNE_TABLE-i));
    }
    size_t s=addrPart[id1];
    addrPart[id1]=addrPart[id];
    addrPart[id]=s;
  }
}

uint8_t TablePartition::cherche1Partition(size_t addrDepart, uint8_t nbMaxPartition) const{
  size_t addr=0xFFFF;
  uint8_t id1=0xFF;
  if( nbMaxPartition==0xFF) nbMaxPartition=this->getNbMaxPartition();

  for(uint8_t id=0;id<nbMaxPartition;id++){
    if( this->getAddrPartition(id) < addr && this->getAddrPartition(id) > addrDepart ) {
      addr=this->getAddrPartition(id);
      id1=id;
    }
  }
  return id1;
}


uint16_t TablePartition::getCRCPartition(uint8_t id) const{
  return eeprom_read_word((uint16_t*)this->getAddressLignePartition(id)+TABLE_PARTITION_LIGNE_POS_CRC);
}
uint16_t TablePartition::calculCRCPartition(uint8_t id) const{
  if( this->getTaillePartition(id) == 0) return 0;
  return crc16Memoire(this->getAddrPartition(id), this->getTaillePartition(id));
}
void TablePartition::ecritCRCPartition(uint8_t id, uint16_t crc){
  if( crc==0xFFFF ) crc=this->calculCRCPartition(id);

  eeprom_update_word((uint16_t*)this->getAddressLignePartition(id)+TABLE_PARTITION_LIGNE_POS_CRC, crc);
}


void TablePartition::printDebug(HardwareSerial* serial){
  this->printDebugInfoG(serial);
  if( this->enteteValide()){
    for(uint8_t id=0;id<this->getNbMaxPartition(); id++){
      this->printDebugPart(serial, id);
    }
  }
}

void TablePartition::printDebugInfoG(HardwareSerial* serial){
  serial->println(F("Table de Partition"));

  serial->print(F("Signature: "));
  char tampon[10];
  eeprom_read_block(tampon, (const void *)TABLE_PARTITION_POS_SIGNATURE, 3);
  serial->print(tampon);
  if( this->readSignature() ) serial->println(F(" OK"));
  else serial->println(F(" erreur !"));

  serial->print(F("CRC lue: "));
  serial->print(this->litCRC());
  serial->print(F("  | CRC calculé: "));
  serial->println(this->calculCRC());

  if( ! this->enteteValide()){
    serial->println("non valide ou corrompue");
    return;
  }

  serial->print(F("nb max de partition: "));
  serial->println(this->getNbMaxPartition());

  serial->print(F("nb de partition: "));
  serial->println(this->getNbPartition());

  serial->print(F("info sup: "));
  eeprom_read_block(tampon, (const void *)TABLE_PARTITION_POS_SIGNATURE, 10);
  for(uint8_t i=0;i<10;i++){
    serial->print(i);
    serial->print(':');
    serial->print(tampon[i],HEX);
    serial->print(' ');
  }
  Serial.println();
  Serial.println();
}
void TablePartition::printDebugPart(HardwareSerial* serial, uint8_t id){
  serial->println(F("Partitions"));
  serial->println();

  serial->print(F(" Part |   nom    | taille | addrDep | CRC | version | autre"));
  for(uint8_t id=0;id<this->getNbPartition();id++){

    const __FlashStringHelper * ec=F(" | ");
    serial->print(' ');
    serial->print(id);
    serial->print(ec);

    if(this->getTaillePartition(id) == 0 ) serial->print(F("NA"));
    else {
      char tampon[8];
      this->getNomPartition(id, tampon);
      serial->print(tampon);
      serial->print(ec);

      serial->print(this->getTaillePartition(id));
      serial->print(ec);
      serial->print(this->getAddrPartition(id));
      serial->print(ec);
      serial->print(this->getVersionPartition(id));
      serial->print(ec);
      eeprom_read_block(tampon, this->getAddressLignePartition(id)+TABLE_PARTITION_LIGNE_POS_RESERVED, 3);
      for(uint8_t i=0;i<3;i++){
        serial->print(i);
        serial->print(':');
        serial->print(tampon[i],HEX);
        serial->print(' ');
      }

    }
    serial->println();
  }
}
