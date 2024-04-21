#include "test_runner.h"
#include "tests.h"


int main()
{
    TestRunner tr;
    RUN_TEST(tr, TestPositionAndStringConversion);
    RUN_TEST(tr, TestPositionToStringInvalid);
    RUN_TEST(tr, TestStringToPositionInvalid);
    RUN_TEST(tr, TestEmpty);
    RUN_TEST(tr, TestInvalidPosition);
    RUN_TEST(tr, TestSetCellPlainText);
    RUN_TEST(tr, TestClearCell);
    RUN_TEST(tr, TestFormulaArithmetic);
    RUN_TEST(tr, TestFormulaReferences);
    RUN_TEST(tr, TestFormulaExpressionFormatting);
    RUN_TEST(tr, TestFormulaReferencedCells);
    RUN_TEST(tr, TestFormulaHandleInsertion);
    RUN_TEST(tr, TestInsertionOverflow);
    RUN_TEST(tr, TestFormulaHandleDeletion);
    RUN_TEST(tr, TestErrorValue);
    RUN_TEST(tr, TestErrorDiv0);
    RUN_TEST(tr, TestEmptyCellTreatedAsZero);
    RUN_TEST(tr, TestFormulaInvalidPosition);
    RUN_TEST(tr, TestCellErrorPropagation);
    RUN_TEST(tr, TestCellsDeletionSimple);
    RUN_TEST(tr, TestCellsDeletion);
    RUN_TEST(tr, TestCellsDeletionAdjacent);
    RUN_TEST(tr, TestPrint);
    RUN_TEST(tr, TestCellReferences);
    RUN_TEST(tr, TestFormulaIncorrect);
    RUN_TEST(tr, TestCellCircularReferences);
    RUN_TEST(tr, TestChangeCellValue);
    RUN_TEST(tr, TestInvalidateCachedValues);
    RUN_TEST(tr, TestDeletedAllCells);
    RUN_TEST(tr, TestInsert);
    RUN_TEST(tr, TestDeletion);
    return 0;
}
