#include "Partition.h"



Partition::Partition(){
  this->setNull();
}

Partition::Partition(const char* nom, modePartition mode){
  this->setNull();
  uint8_t id=TableDesPartitions.getIdPartition(nom);
  if( id != 0xFF ){
    this->init(id, mode);
  }
}

Partition::~Partition(){
  this->close();
}

void Partition::close(){
  if( this->mode == modePartition_Ecriture || this->mode==modePartition_LectureEcriture ) this->actualiseCRC();
  this->mode=modePartition_Close;
}

size_t Partition::getTaille() const{
  return this->taille;
}
modePartition Partition::getMode() const{
  return this->mode;
}

/*
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
*/

bool Partition::setMode(modePartition mode){
  if( this->mode == modePartition_nonInit ) return false; // dans ce cas ->RAF

  if( mode == modePartition_Close ){
    this->close();
    return true;
  }

  if( this->mode == modePartition_Close ) return false;

  if( this->mode == modePartition_Erreur || this->mode == modePartition_Erreur2 ){
    // A FAIRE
    return false; // temporairement
  }

  // A FAIRE
}

size_t Partition::getTailleAct() const{
  return TableDesPartitions.getTailleAct(this->id);
}
bool Partition::setTailleAct(size_t tailleAct){
  if( tailleAct > this->taille ) return false;
  TableDesPartitions.setTailleAct(this->id, tailleAct);

  return true;
}

bool Partition::setCurseur(size_t addr){
  if( addr < this->taille )  {
    this->curseur=addr;
    return true;
  } else return false;
}
size_t Partition::getCurseur() const{
  return this->curseur;
}


bool Partition::insertAt(uint16_t position, size_t taille, bool clear){
  if( position + taille > this->taille ) return false;

  size_t resteAcopier= this->taille - position - taille;

  char tampon[TABLE_PARTITION_TAMPON_COPIE];

  while( resteAcopier > 0){
    size_t tailleCopie=min(TABLE_PARTITION_TAMPON_COPIE, resteAcopier);

    eeprom_read_block(tampon, (const void *)(position+resteAcopier-tailleCopie), tailleCopie);
    eeprom_update_block(tampon, (void *)(position+resteAcopier-tailleCopie+taille), tailleCopie);

    resteAcopier -= tailleCopie;
  }

  if( clear ) this->memset(position, 0, taille);
  return true;
}
bool Partition::deleteAt(uint16_t position, size_t taille){
  if( position + taille > this->taille ) return false;

  size_t resteAcopier= this->taille - position - taille;
  size_t fait=0;

  char tampon[TABLE_PARTITION_TAMPON_COPIE];

  while( resteAcopier > 0){
    size_t tailleCopie=min(TABLE_PARTITION_TAMPON_COPIE, resteAcopier);

    eeprom_read_block(tampon, (const void *)(position+taille+fait), tailleCopie);
    eeprom_update_block(tampon, (void *)(position+fait), tailleCopie);

    fait += tailleCopie;
    resteAcopier -= tailleCopie;
  }
  this->memset(this->taille - taille, 0, taille);
  return true;
}


bool Partition::memset(uint16_t position, uint8_t valeur, size_t taille){
  if( position > this->taille ) return false;

  for(size_t pos=position; (pos < (position+taille) && pos<this->taille); pos++) eeprom_update_byte((uint8_t *)pos, valeur);
}



uint16_t Partition::ecrit(const void* objet, size_t taille, uint16_t position){
  if( mode != modePartition_Ecriture && mode != modePartition_LectureEcriture ) return 0xFFFD;

  if( position + taille > this->taille ){
    return 0xFFFE;
  }
  eeprom_update_block(objet, (void *)(position+this->addr), taille);

  return (position+taille);
}



bool Partition::lit(void* objet, size_t taille){
  size_t newPos=this->lit(objet, taille, this->curseur);
  if( newPos <= TAILLE_EEPROM ) {
    this->curseur += taille;
    return true;
  }
  return false;
}
template <typename T>
bool Partition::ecrit(T& data){
  return (bool)ecrit(&data, sizeof(data));
}
template <typename T>
T Partition::lit(){
  T data = 0;
  this->lit(&data, sizeof(data));
  return data;
}



template <typename T>
bool Partition::ecrit(T& data, uint16_t position){
  return (bool)ecrit(&data, sizeof(data), position);
}
template <typename T>
T Partition::lit(uint16_t position) const{
  T data = 0;
  this->lit(&data, sizeof(data), position);
  return data;
}
uint16_t Partition::lit(void* objet, size_t taille, uint16_t position) const{
  if( mode != modePartition_Lecture && mode != modePartition_LectureEcriture ) return 0xFFFD;

  if( position + taille > this->taille ){
    return 0xFFFE;
  }
  eeprom_read_block(objet, (void *)(position+this->addr), taille);

  return (position+taille);
}
bool Partition::ecrit(const void* objet, size_t taille){
  size_t newPos=this->ecrit(objet, taille, this->curseur);
  if( newPos <= TAILLE_EEPROM ) {
    this->curseur += taille;
    return true;
  }
  return false;
}

bool Partition::controleCRC() const{
  return (this->litCRC() == this->calculCRC() );
}
uint16_t Partition::litCRC() const{
  return TableDesPartitions.getCRCPartition(this->id);
}
uint16_t Partition::calculCRC() const{
  return TableDesPartitions.calculCRCPartition(this->id);
}
void Partition::actualiseCRC(){
  TableDesPartitions.ecritCRCPartition(this->id);
}

void Partition::init(uint8_t id, modePartition mode){
  this->id=id;
  this->addr=TableDesPartitions.getAddrPartition(id);
  this->taille=TableDesPartitions.getTaillePartition(id);
  this->curseur=0;
  this->mode=mode;
}
void Partition::setNull(){
  this->addr=0;
  this->taille=0;
  this->curseur=0;
  this->mode=modePartition_nonInit;
  this->id=0xFF;
}
size_t Partition::getAddr() const{
  return this->addr;
}
