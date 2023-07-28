/****************************************************************************

  Copyright (c) 2022 Ronald Sieber

  Project:      Project independent
  Description:  Implementation of Template Class <SimpleMovingAverage>

  -------------------------------------------------------------------------

  Revision History:

  28.01.2022 -rs:   Start of implementation

****************************************************************************/



/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                                                         //
//          C L A S S   <SimpleMovingAverage>                              //
//                                                                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

template <class T> class SimpleMovingAverage
{

private:

    // Configuration / Default Definitions
    static const int    DEFAULT_SAMPLE_WINDOW_SIZE  = 5;

    // Private Attributes
    T*      m_paSampleWindow;                   // the size of this array represents how many numbers will be used to calculate the average
    T       m_WindowSum;
    T       m_AverageValue;
    int     m_iSampleWindowSize;
    int     m_iSampleWindowIndex;
    bool    m_fAverageSettled;


public:

    // Constructor/Destructor
    SimpleMovingAverage (int iSampleWindowSize_p = DEFAULT_SAMPLE_WINDOW_SIZE);
    ~SimpleMovingAverage ();

    // Public Memebers
    void  Clean ();
    int   GetSampleWindowSize ();
    T     CalcMovingAverage (T NewValue_p);
    T     GetAverageValue ();

};





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                                                         //
//          C O N S T R U C T I O N   /   D E S T R U C T I O N            //
//                                                                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  Constructor
//---------------------------------------------------------------------------

template <class T>  SimpleMovingAverage<T>::SimpleMovingAverage (int iSampleWindowSize_p /* = DEFAULT_SAMPLE_WINDOW_SIZE */)
{

    m_paSampleWindow = new T[iSampleWindowSize_p];
    m_iSampleWindowSize = iSampleWindowSize_p;

    Clean();

    return;

}



//---------------------------------------------------------------------------
//  Destructor
//---------------------------------------------------------------------------

template <class T>  SimpleMovingAverage<T>::~SimpleMovingAverage ()
{

    delete m_paSampleWindow;
    return;

}





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
//                                                                         //
//          P U B L I C    M E T H O D S                                   //
//                                                                         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//  Clean
//---------------------------------------------------------------------------

template <class T>  void  SimpleMovingAverage<T>::Clean ()
{

int  iIdx;


    for (iIdx=0; iIdx<m_iSampleWindowSize; iIdx++)
    {
        m_paSampleWindow[iIdx] = 0;
    }
    m_iSampleWindowIndex = 0;
    m_WindowSum = 0;
    m_AverageValue = 0;

    m_fAverageSettled = false;

    return;

}



//---------------------------------------------------------------------------
//  Get SampleWindow Size
//---------------------------------------------------------------------------

template <class T>  int  SimpleMovingAverage<T>::GetSampleWindowSize ()
{

    return (m_iSampleWindowSize);

}



//---------------------------------------------------------------------------
//  Calculate Moving Average
//---------------------------------------------------------------------------

template <class T>  T  SimpleMovingAverage<T>::CalcMovingAverage (T NewValue_p)
{

T  AverageValue;


    // To avoid calculation errors, the moving average mode is only used after
    // the SampleWindow has been completely filled (m_fAverageSettled==true).
    // Until the first complete filling of the SampleWindow the classic
    // arithmetic average value is calculated.
    if ( m_fAverageSettled )
    {
        // subtract oldest value from previous sum, add the new value
        m_WindowSum = m_WindowSum - m_paSampleWindow[m_iSampleWindowIndex] + NewValue_p;

        // assign new value to the position in the sample window
        m_paSampleWindow[m_iSampleWindowIndex] = NewValue_p;

        // calculate new sliding average value
        AverageValue = m_WindowSum / m_iSampleWindowSize;
    }
    else
    {
        // add the new value
        m_WindowSum = m_WindowSum + NewValue_p;

        // assign new value to the position in the sample window
        m_paSampleWindow[m_iSampleWindowIndex] = NewValue_p;

        // calculate new arithmetic average value
        AverageValue = m_WindowSum / (m_iSampleWindowIndex + 1);
    }


    // shift index pointer to next position
    m_iSampleWindowIndex++;
    if (m_iSampleWindowIndex >= m_iSampleWindowSize)
    {
        m_iSampleWindowIndex = 0;
        m_fAverageSettled = true;
    }

    m_AverageValue = AverageValue;

    return (AverageValue);

}



//---------------------------------------------------------------------------
//  Get last calculated Average Value
//---------------------------------------------------------------------------

template <class T>  T  SimpleMovingAverage<T>::GetAverageValue ()
{

    return (m_AverageValue);

}




// EOF
