namespace Chilano.Common.Design
{
    using System;
    using System.ComponentModel.Design;

    public abstract class DesignerTransactionUtility
    {
        protected DesignerTransactionUtility()
        {
        }

        public static object DoInTransaction(IDesignerHost theHost, string theTransactionName, TransactionAwareParammedMethod theMethod, object theParam)
        {
            DesignerTransaction transaction = null;
            object obj2 = null;
            try
            {
                transaction = theHost.CreateTransaction(theTransactionName);
                obj2 = theMethod(theHost, theParam);
            }
            catch (CheckoutException exception)
            {
                if (exception != CheckoutException.Canceled)
                {
                    throw exception;
                }
            }
            catch
            {
                if (transaction != null)
                {
                    transaction.Cancel();
                    transaction = null;
                }
                throw;
            }
            finally
            {
                if (transaction != null)
                {
                    transaction.Commit();
                }
            }
            return obj2;
        }
    }
}

