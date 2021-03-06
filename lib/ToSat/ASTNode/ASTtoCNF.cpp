/********************************************************************
 * AUTHORS: Mike Katelman
 *
 * BEGIN DATE: November, 2005
 *
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/

#include "stp/AST/AST.h"
#include "stp/STPManager/STPManager.h"
#include "stp/ToSat/ASTNode/ASTtoCNF.h"

namespace stp
{
using std::ostringstream;
using std::cerr;
using std::endl;

bool ASTtoCNF::onChildDoPos(const ASTNode& varphi, unsigned int idx)
{
  bool result = true;

  Kind k = varphi.GetKind();
  switch (k)
  {
    case NOT:
    {
      result = false;
      break;
    }
    case NAND:
    {
      result = false;
      break;
    }
    case NOR:
    {
      result = false;
      break;
    }
    case IMPLIES:
    {
      if (idx == 0)
      {
        result = false;
      }
      break;
    }
    default:
    {
      break;
    }
  }

  return result;
}

bool ASTtoCNF::onChildDoNeg(const ASTNode& varphi, unsigned int idx)
{
  bool result = false;

  Kind k = varphi.GetKind();
  switch (k)
  {
    case NOT:
    {
      result = true;
      break;
    }
    case NAND:
    {
      result = true;
      break;
    }
    case NOR:
    {
      result = true;
      break;
    }
    case XOR:
    {
      result = true;
      break;
    }
    case IFF:
    {
      result = true;
      break;
    }
    case IMPLIES:
    {
      if (idx == 0)
      {
        result = true;
      }
      break;
    }
    case ITE:
    {
      if (idx == 0)
      {
        result = true;
      }
      break;
    }
    default:
    {
      break;
    }
  }

  return result;
}

// utilities for control bits.

void ASTtoCNF::incrementSharesPos(CNFInfo& x)
{
  x.control += ((x.control & 3) < 2) ? 1 : 0;
}

int ASTtoCNF::sharesPos(CNFInfo& x)
{
  return (x.control & 3);
}

void ASTtoCNF::incrementSharesNeg(CNFInfo& x)
{
  x.control += ((x.control & 12) < 8) ? 4 : 0;
}

int ASTtoCNF::sharesNeg(CNFInfo& x)
{
  return ((x.control & 12) >> 2);
}

void ASTtoCNF::setControlBit(CNFInfo& x, unsigned int idx)
{
  x.control |= (1 << idx);
}

bool ASTtoCNF::getControlBit(CNFInfo& x, unsigned int idx)
{
  bool result = false;

  if (x.control & (1 << idx))
  {
    result = true;
  }

  return result;
}

void ASTtoCNF::setIsTerm(CNFInfo& x)
{
  setControlBit(x, 4);
}

bool ASTtoCNF::isTerm(CNFInfo& x)
{
  return getControlBit(x, 4);
}

void ASTtoCNF::setDoRenamePos(CNFInfo& x)
{
  setControlBit(x, 5);
}

bool ASTtoCNF::doRenamePos(CNFInfo& x)
{
  return getControlBit(x, 5);
}

void ASTtoCNF::setWasRenamedPos(CNFInfo& x)
{
  setControlBit(x, 6);
}

bool ASTtoCNF::wasRenamedPos(CNFInfo& x)
{
  return getControlBit(x, 6);
}

void ASTtoCNF::setDoRenameNeg(CNFInfo& x)
{
  setControlBit(x, 7);
}

bool ASTtoCNF::doRenameNeg(CNFInfo& x)
{
  return getControlBit(x, 7);
}

void ASTtoCNF::setWasRenamedNeg(CNFInfo& x)
{
  setControlBit(x, 8);
}

bool ASTtoCNF::wasRenamedNeg(CNFInfo& x)
{
  return getControlBit(x, 8);
}

void ASTtoCNF::setDoSibRenamingPos(CNFInfo& x)
{
  setControlBit(x, 9);
}

bool ASTtoCNF::doSibRenamingPos(CNFInfo& x)
{
  return getControlBit(x, 9);
}

void ASTtoCNF::setDoSibRenamingNeg(CNFInfo& x)
{
  setControlBit(x, 10);
}

bool ASTtoCNF::doSibRenamingNeg(CNFInfo& x)
{
  return getControlBit(x, 10);
}

void ASTtoCNF::setWasVisited(CNFInfo& x)
{
  setControlBit(x, 11);
}

bool ASTtoCNF::wasVisited(CNFInfo& x)
{
  return getControlBit(x, 11);
}

// utilities for clause sets

ClauseList* ASTtoCNF::SINGLETON(const ASTNode& varphi)
{
  ASTNode* copy = ASTNodeToASTNodePtr(varphi);

  ClausePtr clause = new vector<const ASTNode*>();
  clause->push_back(copy);

  ClauseList* psi = new ClauseList();
  psi->push_back(clause);
  return psi;
}

// prep. for cnf conversion

void ASTtoCNF::scanFormula(const ASTNode& varphi, bool isPos)
{
  CNFInfo* x;

  // step 1, get the info associated with this node
  if (info.find(varphi) == info.end())
  {
    x = new CNFInfo();
    info[varphi] = x;
  }
  else
  {
    x = info[varphi];
  }

  // step 2, we only need to know if shares >= 2
  if (isPos && sharesPos(*x) == 2)
  {
    return;
  }

  if (!isPos && sharesNeg(*x) == 2)
  {
    return;
  }

  // step 3, set appropriate information fields
  if (isPos)
  {
    incrementSharesPos(*x);
  }

  if (!isPos)
  {
    incrementSharesNeg(*x);
  }

  // step 4, recurse over children
  if (varphi.isAtom())
  {
    return;
  }
  else if (varphi.isPred())
  {
    for (unsigned int i = 0; i < varphi.GetChildren().size(); i++)
    {
      scanTerm(varphi[i]);
    }
  }
  else
  {
    for (unsigned int i = 0; i < varphi.GetChildren().size(); i++)
    {
      if (onChildDoPos(varphi, i))
      {
        scanFormula(varphi[i], isPos);
      }
      if (onChildDoNeg(varphi, i))
      {
        scanFormula(varphi[i], !isPos);
      }
    }
  }

}

void ASTtoCNF::scanTerm(const ASTNode& varphi)
{
  CNFInfo* x;

  // step 1, get the info associated with this node

  if (info.find(varphi) == info.end())
  {
    x = new CNFInfo();
    info[varphi] = x;
  }
  else
  {
    x = info[varphi];
  }

  // step 2, need two hits because of term ITEs.

  if (sharesPos(*x) == 2)
  {
    return;
  }

  // step 3, set appropriate data fields, always rename
  // term ITEs

  incrementSharesPos(*x);
  setIsTerm(*x);

  // step 4, recurse over children

  if (varphi.isAtom())
  {
    return;
  }
  else if (varphi.isITE())
  {
    scanFormula(varphi[0], true);
    scanFormula(varphi[0], false);
    scanTerm(varphi[1]);
    scanTerm(varphi[2]);
  }
  else
  {
    for (unsigned int i = 0; i < varphi.GetChildren().size(); i++)
    {
      scanTerm(varphi[i]);
    }
  }
}

// main cnf conversion function

void ASTtoCNF::convertFormulaToCNF(const ASTNode& varphi, ClauseList* defs)
{
  CNFInfo* x = info[varphi];

  // divert to special case if term (word-level cnf)

  if (isTerm(*x))
  {
    convertTermForCNF(varphi, defs);
    setWasVisited(*x);
    return;
  }

  // Non-term below

  if (sharesPos(*x) > 0 && !wasVisited(*x))
  {
    convertFormulaToCNFPosCases(varphi, defs);
  }

  if ((x->clausespos != NULL && x->clausespos->size() > 1) &&
    (doSibRenamingPos(*x) || sharesPos(*x) > 1))
  {
    doRenamingPos(varphi, defs);
  }

  if (sharesNeg(*x) > 0 && !wasVisited(*x))
  {
    convertFormulaToCNFNegCases(varphi, defs);
  }

  if ((x->clausesneg != NULL && x->clausesneg->size() > 1) &&
    (doSibRenamingNeg(*x) || sharesNeg(*x) > 1))
  {
    doRenamingNeg(varphi, defs);
  }

  // mark that we've already done the hard work
  setWasVisited(*x);
}

void ASTtoCNF::convertTermForCNF(const ASTNode& varphi, ClauseList* defs)
{
  CNFInfo* x = info[varphi];

  // step 1, done if we've already visited

  if (x->termforcnf != NULL)
  {
    return;
  }

  // step 2, ITE's always get renamed

  if (varphi.isITE())
  {
    x->termforcnf = doRenameITE(varphi, defs);
    reduceMemoryFootprintPos(varphi[0]);
    reduceMemoryFootprintNeg(varphi[0]);
  }
  else if (varphi.isAtom())
  {
    x->termforcnf = ASTNodeToASTNodePtr(varphi);
  }
  else
  {
    ASTVec psis;
    ASTVec::const_iterator it = varphi.GetChildren().begin();
    for (; it != varphi.GetChildren().end(); it++)
    {
      convertTermForCNF(*it, defs);
      psis.push_back(*(info[*it]->termforcnf));
    }

    ASTNode psi = bm->CreateNode(varphi.GetKind(), psis);
    psi.SetValueWidth(varphi.GetValueWidth());
    psi.SetIndexWidth(varphi.GetIndexWidth());
    x->termforcnf = ASTNodeToASTNodePtr(psi);
  }
}

// functions for renaming nodes during cnf conversion

ASTNode* ASTtoCNF::doRenameITE(const ASTNode& varphi, ClauseList* defs)
{
  ASTNode psi;

  // step 1, old "RepLit" code

  ostringstream oss;
  oss << "cnf"
      << "{" << varphi.GetNodeNum() << "}";
  psi = bm->CreateSymbol(oss.str().c_str(), varphi.GetIndexWidth(),
                         varphi.GetValueWidth());

  // step 3, recurse over children

  convertFormulaToCNF(varphi[0], defs);
  convertTermForCNF(varphi[1], defs);
  ASTNode t1 = *(info[varphi[1]]->termforcnf);
  convertTermForCNF(varphi[2], defs);
  ASTNode t2 = *(info[varphi[2]]->termforcnf);

  // step 4, add def clauses

  ClauseList* cl1 = SINGLETON(bm->CreateNode(EQ, psi, t1));
  ClauseList* cl2 = ClauseList::PRODUCT(*(info[varphi[0]]->clausesneg), *cl1);
  DELETE(cl1);
  defs->insert(cl2);
  delete cl2;

  ClauseList* cl3 = SINGLETON(bm->CreateNode(EQ, psi, t2));
  ClauseList* cl4 = ClauseList::PRODUCT(*(info[varphi[0]]->clausespos), *cl3);
  DELETE(cl3);
  defs->insert(cl4);
  delete cl4;

  return ASTNodeToASTNodePtr(psi);
}

void ASTtoCNF::doRenamingPos(const ASTNode& varphi, ClauseList* defs)
{
  CNFInfo* x = info[varphi];

  assert(!wasRenamedPos(*x));

  // step 1, calc new variable

  ostringstream oss;
  oss << "cnf"
      << "{" << varphi.GetNodeNum() << "}";
  ASTNode psi = bm->CreateSymbol(oss.str().c_str(), 0, 0);

  // step 2, add defs

  ASTNode* copy = ASTNodeToASTNodePtr(bm->CreateNode(NOT, psi));
  ClauseList* cl = info[varphi]->clausespos;
  cl->appendToAllClauses(copy);
  defs->insert(cl);
  delete cl;

  // step 3, update info[varphi]

  x->clausespos = SINGLETON(psi);
  setWasRenamedPos(*x);
}

void ASTtoCNF::doRenamingPosXor(const ASTNode& varphi)
{
  CNFInfo* x = info[varphi];

  // step 1, calc new variable

  ostringstream oss;
  oss << "cnf"
      << "{" << varphi.GetNodeNum() << "}";
  ASTNode psi = bm->CreateSymbol(oss.str().c_str(), 0, 0);

  // step 2, add defs

  //     ClauseList* cl1;
  //     cl1 = SINGLETON(bm->CreateNode(NOT, psi));
  //     ClauseList* cl2 = PRODUCT(*(info[varphi]->clausespos), *cl1);
  //     defs->insert(defs->end(), cl2->begin(), cl2->end());
  //     DELETE(info[varphi]->clausespos);
  //     DELETE(cl1);
  //     delete cl2;

  // step 3, update info[varphi]

  x->clausespos = SINGLETON(psi);
  x->clausesneg = SINGLETON(bm->CreateNode(NOT, psi));
  setWasRenamedPos(*x);
}

//   void ASTtoCNF::doRenamingNegXor(const ASTNode& varphi)
//   {
//     CNFInfo* x = info[varphi];

//
//     // step 1, calc new variable
//

//     ostringstream oss;
//     oss << "cnf" << "{" << varphi.GetNodeNum() << "}";
//     ASTNode psi = bm->CreateSymbol(oss.str().c_str());

//
//     // step 2, add defs
//

//     //     ClauseList* cl1;
//     //     cl1 = SINGLETON(bm->CreateNode(NOT, psi));
//     //     ClauseList* cl2 = PRODUCT(*(info[varphi]->clausespos), *cl1);
//     //     defs->insert(defs->end(), cl2->begin(), cl2->end());
//     //     DELETE(info[varphi]->clausespos);
//     //     DELETE(cl1);
//     //     delete cl2;

//
//     // step 3, update info[varphi]
//

//     //x->clausesneg = SINGLETON(bm->CreateNode(NOT,psi));
//     x->clausespos = SINGLETON(bm->CreateNode(NOT,psi));

//     setWasRenamedPos(*x);
//   }//End of doRenamingPos

void ASTtoCNF::doRenamingNeg(const ASTNode& varphi, ClauseList* defs)
{
  CNFInfo* x = info[varphi];

  // step 2, calc new variable

  ostringstream oss;
  oss << "cnf"
      << "{" << varphi.GetNodeNum() << "}";
  ASTNode psi = bm->CreateSymbol(oss.str().c_str(), 0, 0);

  // step 3, add defs

  ASTNode* copy = ASTNodeToASTNodePtr(psi);
  ClauseList* cl = info[varphi]->clausesneg;
  cl->appendToAllClauses(copy);
  defs->insert(cl);
  delete cl;

  // step 4, update info[varphi]

  x->clausesneg = SINGLETON(bm->CreateNode(NOT, psi));
  setWasRenamedNeg(*x);
}

// main switch for individual cnf conversion cases

void ASTtoCNF::convertFormulaToCNFPosCases(const ASTNode& varphi,
                                         ClauseList* defs)
{
  if (varphi.isPred())
  {
    convertFormulaToCNFPosPred(varphi, defs);
    return;
  }

  Kind k = varphi.GetKind();
  switch (k)
  {
    case FALSE:
    {
      convertFormulaToCNFPosFALSE(varphi, defs);
      break;
    }
    case TRUE:
    {
      convertFormulaToCNFPosTRUE(varphi, defs);
      break;
    }
    case BOOLEXTRACT:
    {
      convertFormulaToCNFPosBOOLEXTRACT(varphi, defs);
      break;
    }
    case SYMBOL:
    {
      convertFormulaToCNFPosSYMBOL(varphi, defs);
      break;
    }
    case NOT:
    {
      convertFormulaToCNFPosNOT(varphi, defs);
      break;
    }
    case AND:
    {
      convertFormulaToCNFPosAND(varphi, defs);
      break;
    }
    case NAND:
    {
      convertFormulaToCNFPosNAND(varphi, defs);
      break;
    }
    case OR:
    {
      convertFormulaToCNFPosOR(varphi, defs);
      break;
    }
    case NOR:
    {
      convertFormulaToCNFPosNOR(varphi, defs);
      break;
    }
    case XOR:
    {
      convertFormulaToCNFPosXOR(varphi, defs);
      break;
    }
    case IMPLIES:
    {
      convertFormulaToCNFPosIMPLIES(varphi, defs);
      break;
    }
    case ITE:
    {
      convertFormulaToCNFPosITE(varphi, defs);
      break;
    }
    default:
    {
      fprintf(stderr, "convertFormulaToCNFPosCases: "
                      "doesn't handle kind %d\n",
              k);
      FatalError("");
    }
  }
}

void ASTtoCNF::convertFormulaToCNFNegCases(const ASTNode& varphi,
                                         ClauseList* defs)
{

  if (varphi.isPred())
  {
    convertFormulaToCNFNegPred(varphi, defs);
    return;
  }

  Kind k = varphi.GetKind();
  switch (k)
  {
    case FALSE:
    {
      convertFormulaToCNFNegFALSE(varphi, defs);
      break;
    }
    case TRUE:
    {
      convertFormulaToCNFNegTRUE(varphi, defs);
      break;
    }
    case BOOLEXTRACT:
    {
      convertFormulaToCNFNegBOOLEXTRACT(varphi, defs);
      break;
    }
    case SYMBOL:
    {
      convertFormulaToCNFNegSYMBOL(varphi, defs);
      break;
    }
    case NOT:
    {
      convertFormulaToCNFNegNOT(varphi, defs);
      break;
    }
    case AND:
    {
      convertFormulaToCNFNegAND(varphi, defs);
      break;
    }
    case NAND:
    {
      convertFormulaToCNFNegNAND(varphi, defs);
      break;
    }
    case OR:
    {
      convertFormulaToCNFNegOR(varphi, defs);
      break;
    }
    case NOR:
    {
      convertFormulaToCNFNegNOR(varphi, defs);
      break;
    }
    case XOR:
    {
      convertFormulaToCNFNegXOR(varphi, defs);
      break;
    }
    case IMPLIES:
    {
      convertFormulaToCNFNegIMPLIES(varphi, defs);
      break;
    }
    case ITE:
    {
      convertFormulaToCNFNegITE(varphi, defs);
      break;
    }
    default:
    {
      fprintf(stderr, "convertFormulaToCNFNegCases: "
                      "doesn't handle kind %d\n",
              k);
      FatalError("");
    }
  }
}

// individual cnf conversion cases

void ASTtoCNF::convertFormulaToCNFPosPred(const ASTNode& varphi, ClauseList* defs)
{
  ASTVec psis;

  ASTVec::const_iterator it = varphi.GetChildren().begin();
  for (; it != varphi.GetChildren().end(); it++)
  {
    convertTermForCNF(*it, defs);
    psis.push_back(*(info[*it]->termforcnf));
  }

  info[varphi]->clausespos = SINGLETON(bm->CreateNode(varphi.GetKind(), psis));
}

void ASTtoCNF::convertFormulaToCNFPosFALSE(const ASTNode& varphi,
                                         ClauseList* /*defs*/)
{
  ASTNode dummy_false_var = bm->CreateNode(NOT, dummy_true_var);
  info[varphi]->clausespos = SINGLETON(dummy_false_var);
}

void ASTtoCNF::convertFormulaToCNFPosTRUE(const ASTNode& varphi, ClauseList* /*defs*/)
{
  info[varphi]->clausespos = SINGLETON(dummy_true_var);
}

void ASTtoCNF::convertFormulaToCNFPosBOOLEXTRACT(const ASTNode& varphi,
                                               ClauseList* /*defs*/)
{
  info[varphi]->clausespos = SINGLETON(varphi);
}

void ASTtoCNF::convertFormulaToCNFPosSYMBOL(const ASTNode& varphi,
                                          ClauseList* /*defs*/)
{
  info[varphi]->clausespos = SINGLETON(varphi);
}

void ASTtoCNF::convertFormulaToCNFPosNOT(const ASTNode& varphi, ClauseList* defs)
{
  convertFormulaToCNF(varphi[0], defs);
  info[varphi]->clausespos = ClauseList::COPY(*(info[varphi[0]]->clausesneg));
  reduceMemoryFootprintNeg(varphi[0]);
}

void ASTtoCNF::convertFormulaToCNFPosAND(const ASTNode& varphi, ClauseList* defs)
{
  //****************************************
  // (pos) AND ~> UNION
  //****************************************

  ASTVec::const_iterator it = varphi.GetChildren().begin();
  convertFormulaToCNF(*it, defs);
  ClauseList* psi = ClauseList::COPY(*(info[*it]->clausespos));

  for (it++; it != varphi.GetChildren().end(); it++)
  {
    convertFormulaToCNF(*it, defs);
    CNFInfo* x = info[*it];

    if (sharesPos(*x) == 1)
    {
      psi->insert(x->clausespos);
      delete (x->clausespos);
      x->clausespos = NULL;
      if (x->clausesneg == NULL)
      {
        delete x;
        info.erase(*it);
      }
    }
    else
    {
      ClauseList::INPLACE_UNION(psi, *(x->clausespos));
      reduceMemoryFootprintPos(*it);
    }
  }
  info[varphi]->clausespos = psi;
}

void ASTtoCNF::convertFormulaToCNFPosNAND(const ASTNode& varphi, ClauseList* defs)
{
  bool renamesibs = false;
  ClauseList* clauses;
  ClauseList* psi;
  ClauseList* oldpsi;

  //****************************************
  // (pos) NAND ~> PRODUCT NOT
  //****************************************

  ASTVec::const_iterator it = varphi.GetChildren().begin();
  convertFormulaToCNF(*it, defs);
  clauses = info[*it]->clausesneg;
  if (clauses->size() > 1)
  {
    renamesibs = true;
  }
  psi = ClauseList::COPY(*clauses);
  reduceMemoryFootprintNeg(*it);

  for (it++; it != varphi.GetChildren().end(); it++)
  {
    if (renamesibs)
    {
      setDoSibRenamingNeg(*(info[*it]));
    }
    convertFormulaToCNF(*it, defs);
    clauses = info[*it]->clausesneg;
    if (clauses->size() > 1)
    {
      renamesibs = true;
    }
    oldpsi = psi;
    psi = ClauseList::PRODUCT(*psi, *clauses);
    reduceMemoryFootprintNeg(*it);
    DELETE(oldpsi);
  }

  info[varphi]->clausespos = psi;
}

void ASTtoCNF::convertFormulaToCNFPosOR(const ASTNode& varphi, ClauseList* defs)
{
  bool renamesibs = false;
  ClauseList* clauses;
  ClauseList* psi;
  ClauseList* oldpsi;

  //****************************************
  // (pos) OR ~> PRODUCT
  //****************************************
  ASTVec::const_iterator it = varphi.GetChildren().begin();
  convertFormulaToCNF(*it, defs);
  clauses = info[*it]->clausespos;
  if (clauses->size() > 1)
  {
    renamesibs = true;
  }
  psi = ClauseList::COPY(*clauses);
  reduceMemoryFootprintPos(*it);

  for (it++; it != varphi.GetChildren().end(); it++)
  {
    if (renamesibs)
    {
      setDoSibRenamingPos(*(info[*it]));
    }
    convertFormulaToCNF(*it, defs);
    clauses = info[*it]->clausespos;
    if (clauses->size() > 1)
    {
      renamesibs = true;
    }
    oldpsi = psi;
    psi = ClauseList::PRODUCT(*psi, *clauses);
    reduceMemoryFootprintPos(*it);
    DELETE(oldpsi);
  }

  info[varphi]->clausespos = psi;
}

void ASTtoCNF::convertFormulaToCNFPosNOR(const ASTNode& varphi, ClauseList* defs)
{
  //****************************************
  // (pos) NOR ~> UNION NOT
  //****************************************
  ASTVec::const_iterator it = varphi.GetChildren().begin();
  convertFormulaToCNF(*it, defs);
  ClauseList* psi = ClauseList::COPY(*(info[*it]->clausesneg));
  reduceMemoryFootprintNeg(*it);
  for (it++; it != varphi.GetChildren().end(); it++)
  {
    convertFormulaToCNF(*it, defs);
    ClauseList::INPLACE_UNION(psi, *(info[*it]->clausesneg));
    reduceMemoryFootprintNeg(*it);
  }

  info[varphi]->clausespos = psi;
}

void ASTtoCNF::convertFormulaToCNFPosIMPLIES(const ASTNode& varphi,
                                           ClauseList* defs)
{
  //****************************************
  // (pos) IMPLIES ~> PRODUCT NOT [0] ; [1]
  //****************************************
  CNFInfo* x0 = info[varphi[0]];
  CNFInfo* x1 = info[varphi[1]];
  convertFormulaToCNF(varphi[0], defs);
  if (x0->clausesneg->size() > 1)
  {
    setDoSibRenamingPos(*x1);
  }
  convertFormulaToCNF(varphi[1], defs);
  ClauseList* psi = ClauseList::PRODUCT(*(x0->clausesneg), *(x1->clausespos));
  reduceMemoryFootprintNeg(varphi[0]);
  reduceMemoryFootprintPos(varphi[1]);
  info[varphi]->clausespos = psi;
}

void ASTtoCNF::convertFormulaToCNFPosITE(const ASTNode& varphi, ClauseList* defs)
{
  //****************************************
  // (pos) ITE ~> UNION (PRODUCT NOT [0] ; [1])
  //  ; (PRODUCT [0] ; [2])
  //****************************************
  CNFInfo* x0 = info[varphi[0]];
  CNFInfo* x1 = info[varphi[1]];
  CNFInfo* x2 = info[varphi[2]];
  convertFormulaToCNF(varphi[0], defs);
  if (x0->clausesneg->size() > 1)
  {
    setDoSibRenamingPos(*x1);
  }
  convertFormulaToCNF(varphi[1], defs);
  if (x0->clausespos->size() > 1)
  {
    setDoSibRenamingPos(*x2);
  }
  convertFormulaToCNF(varphi[2], defs);
  ClauseList* psi1 = ClauseList::PRODUCT(*(x0->clausesneg), *(x1->clausespos));
  ClauseList* psi2 = ClauseList::PRODUCT(*(x0->clausespos), *(x2->clausespos));
  ClauseList::NOCOPY_INPLACE_UNION(psi1, psi2);
  reduceMemoryFootprintNeg(varphi[0]);
  reduceMemoryFootprintPos(varphi[1]);
  reduceMemoryFootprintPos(varphi[0]);
  reduceMemoryFootprintPos(varphi[2]);

  info[varphi]->clausespos = psi1;
}

void ASTtoCNF::convertFormulaToCNFPosXOR(const ASTNode& varphi, ClauseList* defs)
{
  ASTVec::const_iterator it = varphi.GetChildren().begin();
  for (; it != varphi.GetChildren().end(); it++)
  {
    convertFormulaToCNF(*it, defs); // make pos and neg clause sets
  }
  ClauseList* psi = convertFormulaToCNFPosXORAux(varphi, 0, defs);
  info[varphi]->clausespos = psi;
  ASTVec::const_iterator it2 = varphi.GetChildren().begin();
  for (; it2 != varphi.GetChildren().end(); it2++)
  {
    reduceMemoryFootprintPos(*it2);
    reduceMemoryFootprintNeg(*it2);
  }
}

ClauseList* ASTtoCNF::convertFormulaToCNFPosXORAux(const ASTNode& varphi,
                                                 unsigned int idx,
                                                 ClauseList* defs)
{
  bool renamesibs;
  ClauseList* psi;
  ClauseList* psi1;
  ClauseList* psi2;

  if (idx == varphi.GetChildren().size() - 2)
  {
    //****************************************
    // (pos) XOR ~> UNION (AND)
    //    (PRODUCT  (OR) [idx]   ;     [idx+1])
    //  ; (PRODUCT NOT   [idx]   ; NOT [idx+1])
    //****************************************
    renamesibs = (info[varphi[idx]]->clausespos)->size() > 1 ? true : false;
    if (renamesibs)
    {
      setDoSibRenamingPos(*info[varphi[idx + 1]]);
    }
    renamesibs = (info[varphi[idx]]->clausesneg)->size() > 1 ? true : false;
    if (renamesibs)
    {
      setDoSibRenamingNeg(*info[varphi[idx + 1]]);
    }

    psi1 = ClauseList::PRODUCT(*(info[varphi[idx]]->clausespos),
                               *(info[varphi[idx + 1]]->clausespos));
    psi2 = ClauseList::PRODUCT(*(info[varphi[idx]]->clausesneg),
                               *(info[varphi[idx + 1]]->clausesneg));
    ClauseList::NOCOPY_INPLACE_UNION(psi1, psi2);

    psi = psi1;
  }
  else
  {
    //****************************************
    // (pos) XOR ~> UNION
    //    (PRODUCT       [idx] ; XOR      [idx+1..])
    //  ; (PRODUCT NOT   [idx] ; NOT XOR  [idx+1..])
    //****************************************
    ClauseList* theta1;
    theta1 = convertFormulaToCNFPosXORAux(varphi, idx + 1, defs);
    renamesibs = theta1->size() > 1 ? true : false;
    if (renamesibs)
    {
      setDoSibRenamingPos(*info[varphi[idx]]);
    }
    ClauseList* theta2;
    theta2 = convertFormulaToCNFNegXORAux(varphi, idx + 1, defs);
    renamesibs = theta2->size() > 1 ? true : false;
    if (renamesibs)
    {
      setDoSibRenamingNeg(*info[varphi[idx]]);
    }

    psi1 = ClauseList::PRODUCT(*(info[varphi[idx]]->clausespos), *theta1);
    psi2 = ClauseList::PRODUCT(*(info[varphi[idx]]->clausesneg), *theta2);
    DELETE(theta1);
    DELETE(theta2);
    ClauseList::NOCOPY_INPLACE_UNION(psi1, psi2);

    psi = psi1;
  }

  return psi;
}

void ASTtoCNF::convertFormulaToCNFNegPred(const ASTNode& varphi, ClauseList* defs)
{

  ASTVec psis;

  ASTVec::const_iterator it = varphi.GetChildren().begin();
  for (; it != varphi.GetChildren().end(); it++)
  {
    convertFormulaToCNF(*it, defs);
    psis.push_back(*(info[*it]->termforcnf));
  }

  info[varphi]->clausesneg =
      SINGLETON(bm->CreateNode(NOT, bm->CreateNode(varphi.GetKind(), psis)));
}

void ASTtoCNF::convertFormulaToCNFNegFALSE(const ASTNode& varphi,
                                         ClauseList* /*defs*/)
{
  info[varphi]->clausesneg = SINGLETON(dummy_true_var);
}

void ASTtoCNF::convertFormulaToCNFNegTRUE(const ASTNode& varphi,
                                          ClauseList* /*defs*/)
{
  ASTNode dummy_false_var = bm->CreateNode(NOT, dummy_true_var);
  info[varphi]->clausesneg = SINGLETON(dummy_false_var);
}

void ASTtoCNF::convertFormulaToCNFNegBOOLEXTRACT(const ASTNode& varphi,
                                               ClauseList* /*defs*/)
{
  ClauseList* psi = SINGLETON(bm->CreateNode(NOT, varphi));
  info[varphi]->clausesneg = psi;
}

void ASTtoCNF::convertFormulaToCNFNegSYMBOL(const ASTNode& varphi,
                                          ClauseList* /*defs*/)
{
  info[varphi]->clausesneg = SINGLETON(bm->CreateNode(NOT, varphi));
}

void ASTtoCNF::convertFormulaToCNFNegNOT(const ASTNode& varphi, ClauseList* defs)
{
  convertFormulaToCNF(varphi[0], defs);
  info[varphi]->clausesneg = ClauseList::COPY(*(info[varphi[0]]->clausespos));
  reduceMemoryFootprintPos(varphi[0]);
}

void ASTtoCNF::convertFormulaToCNFNegAND(const ASTNode& varphi, ClauseList* defs)
{
  bool renamesibs = false;
  ClauseList* clauses;
  ClauseList* psi;
  ClauseList* oldpsi;

  //****************************************
  // (neg) AND ~> PRODUCT NOT
  //****************************************

  ASTVec::const_iterator it = varphi.GetChildren().begin();
  convertFormulaToCNF(*it, defs);
  clauses = info[*it]->clausesneg;
  if (clauses->size() > 1)
  {
    renamesibs = true;
  }
  psi = ClauseList::COPY(*clauses);
  reduceMemoryFootprintNeg(*it);

  for (it++; it != varphi.GetChildren().end(); it++)
  {
    if (renamesibs)
    {
      setDoSibRenamingNeg(*(info[*it]));
    }
    convertFormulaToCNF(*it, defs);
    clauses = info[*it]->clausesneg;
    if (clauses->size() > 1)
    {
      renamesibs = true;
    }

    if (clauses->size() == 1)
      psi->INPLACE_PRODUCT(*clauses);
    else
    {
      oldpsi = psi;
      psi = ClauseList::PRODUCT(*psi, *clauses);
      DELETE(oldpsi);
    }
    reduceMemoryFootprintNeg(*it);
  }

  info[varphi]->clausesneg = psi;
}

void ASTtoCNF::convertFormulaToCNFNegNAND(const ASTNode& varphi, ClauseList* defs)
{
  //****************************************
  // (neg) NAND ~> UNION
  //****************************************
  ASTVec::const_iterator it = varphi.GetChildren().begin();
  convertFormulaToCNF(*it, defs);
  ClauseList* psi = ClauseList::COPY(*(info[*it]->clausespos));
  reduceMemoryFootprintPos(*it);
  for (it++; it != varphi.GetChildren().end(); it++)
  {
    convertFormulaToCNF(*it, defs);
    ClauseList::INPLACE_UNION(psi, *(info[*it]->clausespos));
    reduceMemoryFootprintPos(*it);
  }

  info[varphi]->clausespos = psi;
}

void ASTtoCNF::convertFormulaToCNFNegOR(const ASTNode& varphi, ClauseList* defs)
{
  //****************************************
  // (neg) OR ~> UNION NOT
  //****************************************
  ASTVec::const_iterator it = varphi.GetChildren().begin();
  convertFormulaToCNF(*it, defs);
  ClauseList* psi = ClauseList::COPY(*(info[*it]->clausesneg));
  reduceMemoryFootprintNeg(*it);
  for (it++; it != varphi.GetChildren().end(); it++)
  {
    convertFormulaToCNF(*it, defs);
    CNFInfo* x = info[*it];

    if (sharesNeg(*x) != 1)
    {
      ClauseList::INPLACE_UNION(psi, *(x->clausesneg));
      reduceMemoryFootprintNeg(*it);
    }
    else
    {
      // If this is the only use of "clausesneg", no reason to make a copy.
      psi->insert(x->clausesneg);
      // Copied from reduceMemoryFootprintNeg
      delete x->clausesneg;
      x->clausesneg = NULL;
      if (x->clausespos == NULL)
      {
        delete x;
        info.erase(*it);
      }
    }
  }

  info[varphi]->clausesneg = psi;
}

void ASTtoCNF::convertFormulaToCNFNegNOR(const ASTNode& varphi, ClauseList* defs)
{
  bool renamesibs = false;
  ClauseList* clauses;
  ClauseList* psi;
  ClauseList* oldpsi;

  //****************************************
  // (neg) NOR ~> PRODUCT
  //****************************************
  ASTVec::const_iterator it = varphi.GetChildren().begin();
  convertFormulaToCNF(*it, defs);
  clauses = info[*it]->clausespos;
  if (clauses->size() > 1)
  {
    renamesibs = true;
  }
  psi = ClauseList::COPY(*clauses);
  reduceMemoryFootprintPos(*it);

  for (it++; it != varphi.GetChildren().end(); it++)
  {
    if (renamesibs)
    {
      setDoSibRenamingPos(*(info[*it]));
    }
    convertFormulaToCNF(*it, defs);
    clauses = info[*it]->clausespos;
    if (clauses->size() > 1)
    {
      renamesibs = true;
    }
    oldpsi = psi;
    psi = ClauseList::PRODUCT(*psi, *clauses);
    reduceMemoryFootprintPos(*it);
    DELETE(oldpsi);
  }

  info[varphi]->clausesneg = psi;
}

void ASTtoCNF::convertFormulaToCNFNegIMPLIES(const ASTNode& varphi,
                                           ClauseList* defs)
{
  //****************************************
  // (neg) IMPLIES ~> UNION [0] ; NOT [1]
  //****************************************
  CNFInfo* x0 = info[varphi[0]];
  CNFInfo* x1 = info[varphi[1]];
  convertFormulaToCNF(varphi[0], defs);
  convertFormulaToCNF(varphi[1], defs);
  ClauseList* psi = ClauseList::UNION(*(x0->clausespos), *(x1->clausesneg));
  info[varphi]->clausesneg = psi;
  reduceMemoryFootprintPos(varphi[0]);
  reduceMemoryFootprintNeg(varphi[1]);
}

void ASTtoCNF::convertFormulaToCNFNegITE(const ASTNode& varphi, ClauseList* defs)
{
  //****************************************
  // (neg) ITE ~> UNION (PRODUCT NOT [0] ; NOT [1])
  //  ; (PRODUCT [0] ; NOT [2])
  //****************************************
  CNFInfo* x0 = info[varphi[0]];
  CNFInfo* x1 = info[varphi[1]];
  CNFInfo* x2 = info[varphi[2]];
  convertFormulaToCNF(varphi[0], defs);
  if (x0->clausesneg->size() > 1)
  {
    setDoSibRenamingNeg(*x1);
  }
  convertFormulaToCNF(varphi[1], defs);
  if (x0->clausespos->size() > 1)
  {
    setDoSibRenamingNeg(*x2);
  }
  convertFormulaToCNF(varphi[2], defs);
  ClauseList* psi1 = ClauseList::PRODUCT(*(x0->clausesneg), *(x1->clausesneg));
  ClauseList* psi2 = ClauseList::PRODUCT(*(x0->clausespos), *(x2->clausesneg));
  ClauseList::NOCOPY_INPLACE_UNION(psi1, psi2);
  reduceMemoryFootprintNeg(varphi[0]);
  reduceMemoryFootprintNeg(varphi[1]);
  reduceMemoryFootprintPos(varphi[0]);
  reduceMemoryFootprintNeg(varphi[2]);

  info[varphi]->clausesneg = psi1;
}

void ASTtoCNF::convertFormulaToCNFNegXOR(const ASTNode& varphi, ClauseList* defs)
{
  ASTVec::const_iterator it = varphi.GetChildren().begin();
  for (; it != varphi.GetChildren().end(); it++)
  {
    convertFormulaToCNF(*it, defs); // make pos and neg clause sets
  }
  ClauseList* psi = convertFormulaToCNFNegXORAux(varphi, 0, defs);
  info[varphi]->clausesneg = psi;
  ASTVec::const_iterator it2 = varphi.GetChildren().begin();
  for (; it2 != varphi.GetChildren().end(); it2++)
  {
    reduceMemoryFootprintPos(*it2);
    reduceMemoryFootprintNeg(*it2);
  }
}

ClauseList* ASTtoCNF::convertFormulaToCNFNegXORAux(const ASTNode& varphi,
                                                 unsigned int idx,
                                                 ClauseList* defs)
{
  bool renamesibs;
  ClauseList* psi;
  ClauseList* psi1;
  ClauseList* psi2;

  if (idx == varphi.GetChildren().size() - 2)
  {

    //****************************************
    // (neg) XOR ~> UNION
    //    (PRODUCT NOT   [idx]   ;     [idx+1])
    //  ; (PRODUCT       [idx]   ; NOT [idx+1])
    //****************************************
    convertFormulaToCNF(varphi[idx], defs);
    renamesibs = (info[varphi[idx]]->clausesneg)->size() > 1 ? true : false;
    if (renamesibs)
    {
      setDoSibRenamingPos(*info[varphi[idx + 1]]);
    }

    convertFormulaToCNF(varphi[idx], defs);
    renamesibs = (info[varphi[idx]]->clausespos)->size() > 1 ? true : false;
    if (renamesibs)
    {
      setDoSibRenamingNeg(*info[varphi[idx + 1]]);
    }

    psi1 = ClauseList::PRODUCT(*(info[varphi[idx]]->clausesneg),
                               *(info[varphi[idx + 1]]->clausespos));
    psi2 = ClauseList::PRODUCT(*(info[varphi[idx]]->clausespos),
                               *(info[varphi[idx + 1]]->clausesneg));
    ClauseList::NOCOPY_INPLACE_UNION(psi1, psi2);

    psi = psi1;
  }
  else
  {
    //****************************************
    // (neg) XOR ~> UNION
    //    (PRODUCT NOT   [idx] ; XOR      [idx+1..])
    //  ; (PRODUCT       [idx] ; NOT XOR  [idx+1..])
    //****************************************
    ClauseList* theta1;
    theta1 = convertFormulaToCNFPosXORAux(varphi, idx + 1, defs);
    renamesibs = theta1->size() > 1 ? true : false;
    if (renamesibs)
    {
      setDoSibRenamingNeg(*info[varphi[idx]]);
    }
    convertFormulaToCNF(varphi[idx], defs);

    ClauseList* theta2;
    theta2 = convertFormulaToCNFNegXORAux(varphi, idx + 1, defs);
    renamesibs = theta2->size() > 1 ? true : false;
    if (renamesibs)
    {
      setDoSibRenamingPos(*info[varphi[idx]]);
    }

    psi1 = ClauseList::PRODUCT(*(info[varphi[idx]]->clausesneg), *theta1);
    psi2 = ClauseList::PRODUCT(*(info[varphi[idx]]->clausespos), *theta2);
    DELETE(theta1);
    DELETE(theta2);
    ClauseList::NOCOPY_INPLACE_UNION(psi1, psi2);

    psi = psi1;
  }

  return psi;
}

// utilities for reclaiming memory.

void ASTtoCNF::reduceMemoryFootprintPos(const ASTNode& varphi)
{
  CNFInfo* x = info[varphi];
  if (sharesPos(*x) == 1)
  {
    DELETE(x->clausespos);
    if (x->clausesneg == NULL)
    {
      delete x;
      info.erase(varphi);
    }
  }
}

void ASTtoCNF::reduceMemoryFootprintNeg(const ASTNode& varphi)
{
  CNFInfo* x = info[varphi];
  if (sharesNeg(*x) == 1)
  {
    DELETE(x->clausesneg);
    if (x->clausespos == NULL)
    {
      delete x;
      info.erase(varphi);
    }
  }
}

ASTNode* ASTtoCNF::ASTNodeToASTNodePtr(const ASTNode& varphi)
{
  ASTNode* psi;

  if (store.find(varphi) != store.end())
  {
    psi = store[varphi];
  }
  else
  {
    psi = new ASTNode(varphi);
    store[varphi] = psi;
  }

  return psi;
}

void ASTtoCNF::cleanup(const ASTNode& varphi)
{
  delete info[varphi]->clausespos;
  CNFInfo* toDelete = info[varphi]; // get the thing to delete.
  info.erase(varphi);               // remove it from the hashtable
  delete toDelete;

  ASTNodeToCNFInfoMap::const_iterator it1 = info.begin();
  for (; it1 != info.end(); it1++)
  {
    CNFInfo* x = it1->second;
    if (x->clausespos != NULL)
    {
      DELETE(x->clausespos);
    }
    if (x->clausesneg != NULL)
    {
      if (!isTerm(*x))
      {
        DELETE(x->clausesneg);
      }
    }
    delete x;
  }

  info.clear();
}

ASTtoCNF::ASTtoCNF(STPMgr* bmgr)
{
  bm = bmgr;
  dummy_true_var = bmgr->CreateFreshVariable(0, 0, "*TrueDummy*");
}

ASTtoCNF::~ASTtoCNF()
{
  ASTNodeToASTNodePtrMap::const_iterator it1 = store.begin();
  for (; it1 != store.end(); it1++)
  {
    delete it1->second;
  }
  store.clear();
}

// top-level conversion function
ClauseList* ASTtoCNF::convertToCNF(const ASTNode& varphi)
{
  bm->GetRunTimes()->start(RunTimes::CNFConversion);
  scanFormula(varphi, true);
  ClauseList* defs = SINGLETON(dummy_true_var);
  convertFormulaToCNF(varphi, defs);
  ClauseList* top = info[varphi]->clausespos;
  defs->insertAtFront(top);

  cleanup(varphi);
  bm->GetRunTimes()->stop(RunTimes::CNFConversion);
  if (bm->UserFlags.stats_flag)
  {
    cerr << "\nPrinting: After CNF conversion: " << endl;
    cerr << "Number of clauses:" << defs->size() << endl;
    //PrintClauseList(cout, *defs);
  }

  return defs;
}

void ASTtoCNF::DELETE(ClauseList*& varphi)
{
  varphi->deleteJustVectors();
  delete varphi;
  varphi = NULL;
}

} // end namespace stp
