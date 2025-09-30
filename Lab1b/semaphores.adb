-- Package: Semaphores
--
-- ==> Complete the code at the indicated places

package body Semaphores is
   protected body CountingSemaphore is
      entry Wait when Count > 0 is -- To be completed
      begin
         Count := Count - 1;
      end Wait;
      entry Signal when Count < MaxCount is -- To be completed
      begin
         Count := Count + 1;
      end Signal;
   end CountingSemaphore;
end Semaphores;
